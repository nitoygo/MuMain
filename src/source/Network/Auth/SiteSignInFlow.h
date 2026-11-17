#pragma once

#include "Network/Auth/LoginTicketSource.h"
#include "Network/Auth/SiteSignIn.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace Network::Auth
{
class LoopbackListener;

struct SiteHttpResponse
{
    // 0 when the request got no answer.
    int status = 0;
    std::string body;
};

using FormFields = std::vector<std::pair<std::string, std::string>>;

// POSTs a form to the site; the target's adapter brings the HTTP client, tests a fake. Called on a worker thread; a
// set cancel flag should end a request early.
class ISiteHttp
{
public:
    virtual ~ISiteHttp() = default;
    virtual SiteHttpResponse PostForm(const std::string& url, const FormFields& fields,
                                      const std::atomic_bool& cancel) = 0;
};

// Opens a URL in the player's browser; called on the main thread.
class ISystemBrowser
{
public:
    virtual ~ISystemBrowser() = default;
    virtual bool Open(const std::string& url) = 0;
};

struct SiteSignInTimings
{
    // How long the player has to finish in the browser.
    std::chrono::milliseconds browserWait{std::chrono::minutes(5)};
    // One second of the site's device-code interval and expiry; tests shorten it.
    std::chrono::milliseconds second{std::chrono::seconds(1)};
    // 0: a port the system picks. Tests name a taken one to force the device code.
    unsigned short loopbackPort = 0;
};

// Runs one sign-in at the site on a worker thread: the browser with the loopback redirect (or the device code when
// the game can't listen, or no browser opens), or a refresh token's renewal. Start and take the answer on the main
// thread; destroying it cancels and waits for the worker.
class SiteSignInFlow
{
public:
    SiteSignInFlow(ISiteHttp& http, ISystemBrowser& browser, std::string site, std::string deviceName,
                   SiteSignInTimings timings = {});
    ~SiteSignInFlow();

    SiteSignInFlow(const SiteSignInFlow&) = delete;
    SiteSignInFlow& operator=(const SiteSignInFlow&) = delete;

    void SignInWithBrowser();
    void Refresh(std::string refreshToken);
    // Stops the running sign-in and drops its answer.
    void Cancel();

    // The finished sign-in's answer, once.
    std::optional<SiteSignIn::TokenAnswer> TakeAnswer();
    LoginPrompt Prompt() const;

private:
    void Start(std::function<SiteSignIn::TokenAnswer()> work);
    SiteSignIn::TokenAnswer AwaitBrowser(LoopbackListener& listener, const std::string& redirectUri,
                                         const std::string& state, const std::string& verifier);
    SiteSignIn::TokenAnswer SignInWithDeviceCode();
    SiteSignIn::TokenAnswer PostToken(FormFields fields);
    // False when cancelled during the wait.
    bool Sleep(std::chrono::milliseconds wait);
    void SetPrompt(LoginPrompt prompt);

    ISiteHttp& m_http;
    ISystemBrowser& m_browser;
    std::string m_site;
    std::string m_deviceName;
    SiteSignInTimings m_timings;

    std::thread m_worker;
    std::atomic_bool m_cancel{false};
    mutable std::mutex m_mutex;
    std::condition_variable m_wake;
    std::optional<SiteSignIn::TokenAnswer> m_answer;
    LoginPrompt m_prompt;
};
} // namespace Network::Auth
