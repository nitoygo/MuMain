#include "stdafx.h"

#include "Network/Auth/SiteSignInFlow.h"

#include "Network/Auth/DevicePoll.h"
#include "Network/Auth/LoopbackListener.h"

#include <memory>

namespace Network::Auth
{
namespace
{
using namespace SiteSignIn;

constexpr int kHttpNotFound = 404;
constexpr int kHttpOk = 200;
constexpr const char* kReturnToGame =
    "<!doctype html><meta charset=utf-8><title>Signed in</title><p>You are signed in. You can return to the game.</p>";
constexpr const char* kSignInStopped =
    "<!doctype html><meta charset=utf-8><title>Not signed in</title><p>The game was not signed in. You can return to "
    "the game and try again.</p>";
} // namespace

SiteSignInFlow::SiteSignInFlow(ISiteHttp& http, ISystemBrowser& browser, std::string site, std::string deviceName,
                               SiteSignInTimings timings)
    : m_http(http), m_browser(browser), m_site(std::move(site)), m_deviceName(std::move(deviceName)), m_timings(timings)
{
}

SiteSignInFlow::~SiteSignInFlow()
{
    Cancel();
}

void SiteSignInFlow::SignInWithBrowser()
{
    Cancel();
    const auto pkce = NewPkce();
    const std::string state = NewState();
    if (!pkce || state.empty())
    {
        Start([] { return TokenAnswer{.error = NoRandom}; });
        return;
    }

    auto listener = std::make_shared<LoopbackListener>();
    const bool listening = listener->Listen(m_timings.loopbackPort);
    const std::string redirectUri = LoopbackRedirect(listener->Port());
    if (!listening || !m_browser.Open(AuthorizeUrl(m_site, redirectUri, state, pkce->challenge)))
    {
        listener->Close();
        Start([this] { return SignInWithDeviceCode(); });
        return;
    }

    SetPrompt({.inBrowser = true});
    Start([this, listener, redirectUri, state, verifier = pkce->verifier]
          { return AwaitBrowser(*listener, redirectUri, state, verifier); });
}

void SiteSignInFlow::Refresh(std::string refreshToken)
{
    Cancel();
    Start([this, token = std::move(refreshToken)]
          { return PostToken({{"grant_type", "refresh_token"}, {"refresh_token", token}}); });
}

void SiteSignInFlow::Cancel()
{
    {
        std::lock_guard lock(m_mutex);
        m_cancel = true;
    }
    m_wake.notify_all();
    if (m_worker.joinable())
        m_worker.join();
    std::lock_guard lock(m_mutex);
    m_answer.reset();
    m_prompt = {};
}

std::optional<TokenAnswer> SiteSignInFlow::TakeAnswer()
{
    std::optional<TokenAnswer> answer;
    {
        std::lock_guard lock(m_mutex);
        answer = std::exchange(m_answer, std::nullopt);
    }
    // The answer is the worker's last step, so it is about to end.
    if (answer && m_worker.joinable())
        m_worker.join();
    return answer;
}

LoginPrompt SiteSignInFlow::Prompt() const
{
    std::lock_guard lock(m_mutex);
    return m_prompt;
}

void SiteSignInFlow::Start(std::function<TokenAnswer()> work)
{
    m_cancel = false;
    m_worker = std::thread(
        [this, work = std::move(work)]
        {
            TokenAnswer answer = work();
            std::lock_guard lock(m_mutex);
            m_answer = std::move(answer);
            m_prompt = {};
        });
}

TokenAnswer SiteSignInFlow::AwaitBrowser(LoopbackListener& listener, const std::string& redirectUri,
                                         const std::string& state, const std::string& verifier)
{
    const auto deadline = std::chrono::steady_clock::now() + m_timings.browserWait;
    while (const auto request = listener.NextRequest(m_cancel, deadline))
    {
        const Callback callback = ParseCallback(*request, state);
        if (callback.outcome == CallbackOutcome::NotCallback)
        {
            listener.Answer(kHttpNotFound, "");
            continue;
        }

        // One callback ends the listening, whatever it says: nothing else is accepted on that port.
        listener.Answer(kHttpOk, callback.outcome == CallbackOutcome::Code ? kReturnToGame : kSignInStopped);
        listener.Close();
        if (callback.outcome != CallbackOutcome::Code)
            return {.error = callback.outcome == CallbackOutcome::Denied ? "access_denied" : BadCallback};
        return PostToken({{"grant_type", "authorization_code"},
                          {"code", callback.code},
                          {"code_verifier", verifier},
                          {"redirect_uri", redirectUri}});
    }
    return {.error = m_cancel ? Cancelled : TimedOut};
}

TokenAnswer SiteSignInFlow::SignInWithDeviceCode()
{
    const SiteHttpResponse started =
        m_http.PostForm(m_site + DevicePath, {{"client_id", ClientId}, {"device_name", m_deviceName}}, m_cancel);
    const auto authorization = ParseDeviceAuthorization(started.status, started.body);
    if (!authorization)
        return {.error =
                    started.status == 0 ? NetworkError : "device code refused, HTTP " + std::to_string(started.status)};

    SetPrompt({.url = authorization->verificationUri, .code = authorization->userCode});
    DevicePoll poll(authorization->intervalSeconds, authorization->expiresInSeconds);
    const auto startedAt = std::chrono::steady_clock::now();
    while (Sleep(poll.IntervalSeconds() * m_timings.second))
    {
        const auto elapsed = std::chrono::steady_clock::now() - startedAt;
        if (poll.Expired(static_cast<int>(elapsed / m_timings.second)))
            return {.error = "expired_token"};

        TokenAnswer answer = PostToken({{"grant_type", DeviceGrantType}, {"device_code", authorization->deviceCode}});
        if (poll.OnAnswer(answer) != DevicePoll::Step::Wait)
            return answer;
    }
    return {.error = Cancelled};
}

TokenAnswer SiteSignInFlow::PostToken(FormFields fields)
{
    fields.emplace_back("client_id", ClientId);
    if (!m_deviceName.empty())
        fields.emplace_back("device_name", m_deviceName);
    const SiteHttpResponse response = m_http.PostForm(m_site + TokenPath, fields, m_cancel);
    return ParseTokenAnswer(response.status, response.body);
}

bool SiteSignInFlow::Sleep(std::chrono::milliseconds wait)
{
    std::unique_lock lock(m_mutex);
    return !m_wake.wait_for(lock, wait, [this] { return m_cancel.load(); });
}

void SiteSignInFlow::SetPrompt(LoginPrompt prompt)
{
    std::lock_guard lock(m_mutex);
    m_prompt = std::move(prompt);
}
} // namespace Network::Auth
