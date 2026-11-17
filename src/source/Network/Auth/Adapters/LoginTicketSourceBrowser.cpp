// The game's website signs the player in through the system browser (SiteSignInFlow): with a password or Discord on
// the site, never in the game. The site's refresh token renews tickets on reconnect and, with "remember me", is kept
// encrypted for "Continue as <name>" on the next start. Off while config.ini names no SiteUrl.
#include "stdafx.h"

#include "Network/Auth/LoginTicketSource.h"
#include "Network/Auth/SavedSignIn.h"
#include "Network/Auth/SiteSignIn.h"
#include "Network/Auth/SiteSignInFlow.h"

#include "Core/Platform/PlatformCrypto.h"
#include "Core/Time/FrameTimerScheduler.h"
#include "Core/Utilities/StringUtils.h"
#include "Data/GameConfig/GameConfig.h"

#include <SDL3/SDL_misc.h>
#include <curl/curl.h>

#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace Network::Auth
{
namespace
{
constexpr const char* kSiteProvider = "site";
constexpr const char* kSavedProvider = "site-saved";
constexpr const char* kBusy = "busy";
constexpr const char* kNoSignIn = "no saved sign-in";
constexpr long kConnectTimeoutMs = 5000;
constexpr long kRequestTimeoutMs = 15000;
// Far more than a token answer; a site that sends more is cut off.
constexpr std::size_t kMaxAnswerBytes = 64 * 1024;

void EnsureCurlGlobalInit()
{
    static std::once_flag once;
    std::call_once(once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

size_t AppendAnswer(char* data, size_t size, size_t count, void* answer)
{
    auto& body = *static_cast<std::string*>(answer);
    const size_t bytes = size * count;
    if (body.size() + bytes > kMaxAnswerBytes)
        return 0;
    body.append(data, bytes);
    return bytes;
}

int AbortWhenCancelled(void* cancel, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
{
    return static_cast<const std::atomic_bool*>(cancel)->load() ? 1 : 0;
}

class CurlSiteHttp final : public ISiteHttp
{
public:
    SiteHttpResponse PostForm(const std::string& url, const FormFields& fields, const std::atomic_bool& cancel) override
    {
        EnsureCurlGlobalInit();
        CURL* curl = curl_easy_init();
        if (curl == nullptr)
            return {};

        std::string form;
        for (const auto& [name, value] : fields)
            form += (form.empty() ? "" : "&") + name + "=" + SiteSignIn::UrlEncode(value);

        SiteHttpResponse response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, form.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &AppendAnswer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, kConnectTimeoutMs);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, kRequestTimeoutMs);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &AbortWhenCancelled);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, const_cast<std::atomic_bool*>(&cancel));

        long status = 0;
        if (curl_easy_perform(curl) == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_cleanup(curl);
        response.status = static_cast<int>(status);
        return response;
    }
};

class SdlSystemBrowser final : public ISystemBrowser
{
public:
    bool Open(const std::string& url) override
    {
        return SDL_OpenURL(url.c_str());
    }
};

// SiteUrl without a trailing slash; empty unless it is an http(s) address.
std::string ConfiguredSite()
{
    std::string site = StringUtils::WideToNarrow(GameConfig::GetInstance().GetSiteUrl().c_str());
    while (!site.empty() && site.back() == '/')
        site.pop_back();
    const bool web = site.rfind("https://", 0) == 0 || site.rfind("http://", 0) == 0;
    return web ? site : std::string{};
}

// How the site lists this computer under "Devices signed in"; the site has its own default.
std::string DeviceName()
{
    for (const char* variable : {"COMPUTERNAME", "HOSTNAME"})
    {
        if (const char* value = std::getenv(variable); value != nullptr && *value != '\0')
            return value;
    }
    return {};
}

SavedSignInStore MakeStore()
{
    return SavedSignInStore(
        &mu_encrypt_blob, &mu_decrypt_blob, [] { return GameConfig::GetInstance().GetSavedSignIn(); },
        [](const std::wstring& sealed) { GameConfig::GetInstance().SetSavedSignIn(sealed); });
}

class BrowserLoginTicketSource final : public ILoginTicketSource
{
public:
    std::vector<LoginProvider> Providers() const override
    {
        const std::string site = ConfiguredSite();
        if (site.empty())
            return {};

        std::vector<LoginProvider> providers;
        // Started by the player's earlier "remember me": continues once per run without a click, as on the web.
        if (const auto saved = m_store.Load())
            providers.push_back(
                {.id = kSavedProvider, .displayName = "Continue as " + saved->name, .autoContinue = true});
        providers.push_back({.id = kSiteProvider, .displayName = "Sign in with " + SiteSignIn::SiteHost(site)});
        return providers;
    }

    void Acquire(const std::string& providerId, LoginTicketCallback done) override
    {
        if (providerId == kSavedProvider)
        {
            Renew(std::move(done));
            return;
        }
        if (providerId != kSiteProvider)
        {
            done({.error = "unknown provider " + providerId});
            return;
        }
        if (Flow(std::move(done)))
            m_flow->SignInWithBrowser();
    }

    void Renew(LoginTicketCallback done) override
    {
        std::string token = m_refreshToken;
        if (token.empty())
        {
            if (const auto saved = m_store.Load())
                token = saved->refreshToken;
        }
        if (token.empty())
        {
            done({.error = kNoSignIn});
            return;
        }
        if (Flow(std::move(done)))
            m_flow->Refresh(std::move(token));
    }

    void Cancel() override
    {
        Core::Time::FrameTimerScheduler::Instance().Kill(TimerId());
        m_waiting = nullptr;
        m_flow.reset();
    }

    LoginPrompt Prompt() const override
    {
        return m_flow ? m_flow->Prompt() : LoginPrompt{};
    }

private:
    // A new flow for one sign-in at a time; false (and done told) while another runs or without a site.
    bool Flow(LoginTicketCallback done)
    {
        if (m_waiting)
        {
            done({.error = kBusy});
            return false;
        }
        const std::string site = ConfiguredSite();
        if (site.empty())
        {
            done({.error = "no SiteUrl in config.ini"});
            return false;
        }

        m_waiting = std::move(done);
        m_flow = std::make_unique<SiteSignInFlow>(m_http, m_browser, site, DeviceName());
        // The answer is handed over on the main loop, never from the worker thread.
        Core::Time::FrameTimerScheduler::Instance().SetRepeating(TimerId(), 0, [this] { Deliver(); });
        return true;
    }

    void Deliver()
    {
        auto answer = m_flow ? m_flow->TakeAnswer() : std::nullopt;
        if (!answer)
            return;

        Core::Time::FrameTimerScheduler::Instance().Kill(TimerId());
        Keep(*answer);
        auto done = std::exchange(m_waiting, nullptr);
        if (answer->ticket.empty())
            done({.error = answer->error.empty() ? std::string("no ticket") : answer->error});
        else
            done({.token = answer->ticket});
    }

    // A rotated refresh token replaces the old one at once: the site treats the old one coming back as stolen.
    void Keep(const SiteSignIn::TokenAnswer& answer)
    {
        if (answer.error == SiteSignIn::InvalidGrant)
        {
            m_refreshToken.clear();
            m_store.Clear();
            return;
        }
        if (!answer.Succeeded())
            return;

        m_refreshToken = answer.refreshToken;
        if (GameConfig::GetInstance().GetRememberMe())
            m_store.Save({.name = answer.name, .refreshToken = answer.refreshToken});
        else
            m_store.Clear();
    }

    Core::Time::FrameTimerScheduler::TimerId TimerId() const
    {
        return reinterpret_cast<Core::Time::FrameTimerScheduler::TimerId>(this);
    }

    CurlSiteHttp m_http;
    SdlSystemBrowser m_browser;
    SavedSignInStore m_store = MakeStore();
    // This run's sign-in, for reconnects even without "remember me".
    std::string m_refreshToken;
    std::unique_ptr<SiteSignInFlow> m_flow;
    LoginTicketCallback m_waiting;
};
} // namespace

ILoginTicketSource& LoginTicketSource()
{
    static BrowserLoginTicketSource source;
    return source;
}
} // namespace Network::Auth
