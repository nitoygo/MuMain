#include "doctest.h"

#include "Core/Platform/Adapters/OpenSslCredentials.h"
#include "Core/Platform/WinSock.h"
#include "Network/Auth/DevicePoll.h"
#include "Network/Auth/LoopbackListener.h"
#include "Network/Auth/SavedSignIn.h"
#include "Network/Auth/SiteSignIn.h"
#include "Network/Auth/SiteSignInFlow.h"

#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace Network::Auth;
using namespace Network::Auth::SiteSignIn;

namespace
{
// RFC 7636 appendix B.
constexpr const char* kVerifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
constexpr const char* kChallenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
constexpr const char* kSite = "https://mu.example.com";
constexpr std::chrono::seconds kFlowLimit{10};

std::string QueryParam(const std::string& url, const std::string& name)
{
    const std::size_t start = url.find(name + "=");
    if (start == std::string::npos)
        return {};
    const std::size_t from = start + name.size() + 1;
    std::string value = url.substr(from, url.find('&', from) - from);
    std::string decoded;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '%' && i + 2 < value.size())
        {
            decoded += static_cast<char>(std::stoi(value.substr(i + 1, 2), nullptr, 16));
            i += 2;
        }
        else
            decoded += value[i];
    }
    return decoded;
}

// What a browser does with the loopback: one HTTP request, and the page it gets back.
std::string BrowserRequest(unsigned short port, const std::string& target)
{
    WSADATA data{};
    WSAStartup(MAKEWORD(2, 2), &data);
    const SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    std::string page;
    if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0)
    {
        const std::string request = "GET " + target + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
        send(socket, request.data(), static_cast<int>(request.size()), 0);
        char chunk[1024];
        int read = 0;
        while ((read = recv(socket, chunk, sizeof(chunk), 0)) > 0)
            page.append(chunk, static_cast<std::size_t>(read));
    }
    closesocket(socket);
    WSACleanup();
    return page;
}

class FakeHttp final : public ISiteHttp
{
public:
    SiteHttpResponse PostForm(const std::string& url, const FormFields& fields, const std::atomic_bool&) override
    {
        std::lock_guard lock(mutex);
        posts.push_back({url, std::map<std::string, std::string>(fields.begin(), fields.end())});
        if (answers.empty())
            return {};
        SiteHttpResponse answer = answers.front();
        answers.pop_front();
        return answer;
    }

    struct Post
    {
        std::string url;
        std::map<std::string, std::string> fields;
    };

    std::mutex mutex;
    std::deque<SiteHttpResponse> answers;
    std::vector<Post> posts;
};

// Follows the authorize URL like a browser whose player clicked Allow, after a stray request of its own.
class AllowingBrowser final : public ISystemBrowser
{
public:
    bool Open(const std::string& url) override
    {
        opened = url;
        const std::string redirect = QueryParam(url, "redirect_uri");
        const auto port = static_cast<unsigned short>(std::stoi(redirect.substr(redirect.rfind(':') + 1)));
        const std::string state = stateOverride.empty() ? QueryParam(url, "state") : stateOverride;
        player = std::thread(
            [this, port, state]
            {
                strayPage = BrowserRequest(port, "/favicon.ico");
                callbackPage = BrowserRequest(port, "/callback?code=the%2Bcode&state=" + state);
            });
        return true;
    }

    ~AllowingBrowser() override
    {
        if (player.joinable())
            player.join();
    }

    std::string stateOverride;
    std::string opened;
    std::string strayPage;
    std::string callbackPage;
    std::thread player;
};

class NoBrowser final : public ISystemBrowser
{
public:
    bool Open(const std::string&) override
    {
        return false;
    }
};

std::optional<TokenAnswer> AwaitAnswer(SiteSignInFlow& flow)
{
    const auto until = std::chrono::steady_clock::now() + kFlowLimit;
    while (std::chrono::steady_clock::now() < until)
    {
        if (auto answer = flow.TakeAnswer())
            return answer;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return std::nullopt;
}

SiteHttpResponse SignedIn()
{
    return {200, R"({"refresh_token":"refresh-1","name":"player","ticket":"ticket-1","ticket_expires_at":"x"})"};
}

bool TestEncrypt(const void* input, std::size_t size, std::vector<std::uint8_t>& output)
{
    return mu_encrypt_blob_with_key_material("test:1", input, size, output);
}

bool TestDecrypt(const void* input, std::size_t size, std::vector<std::uint8_t>& output)
{
    return mu_decrypt_blob_with_key_material("test:1", input, size, output);
}

bool OtherHostDecrypt(const void* input, std::size_t size, std::vector<std::uint8_t>& output)
{
    return mu_decrypt_blob_with_key_material("test:2", input, size, output);
}
} // namespace

TEST_CASE("The PKCE challenge is the RFC 7636 S256 of the verifier [network][site-sign-in]")
{
    CHECK(ChallengeOf(kVerifier) == kChallenge);

    const auto pkce = NewPkce();
    REQUIRE(pkce.has_value());
    CHECK(pkce->verifier.size() == 43);
    CHECK(pkce->challenge == ChallengeOf(pkce->verifier));
    CHECK(pkce->verifier.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") ==
          std::string::npos);
    CHECK(NewPkce()->verifier != pkce->verifier);
    CHECK(NewState().size() == 22);
    CHECK(NewState() != NewState());
}

TEST_CASE("The authorize URL carries the loopback, state and challenge encoded [network][site-sign-in]")
{
    const std::string url = AuthorizeUrl(kSite, LoopbackRedirect(50123), "s/t", kChallenge);

    CHECK(url.rfind("https://mu.example.com/game-auth/authorize?client_id=mu-client&response_type=code&", 0) == 0);
    CHECK(url.find("redirect_uri=http%3A%2F%2F127.0.0.1%3A50123%2Fcallback") != std::string::npos);
    CHECK(url.find("state=s%2Ft") != std::string::npos);
    CHECK(url.find("code_challenge_method=S256") != std::string::npos);
    CHECK(SiteHost("https://mu.example.com:8443/x") == "mu.example.com");
}

TEST_CASE("The callback is taken only with the state the game sent [network][site-sign-in]")
{
    const auto valid = ParseCallback("GET /callback?code=a%2Bb&state=s1 HTTP/1.1\r\nHost: x\r\n\r\n", "s1");
    CHECK(valid.outcome == CallbackOutcome::Code);
    CHECK(valid.code == "a+b");

    CHECK(ParseCallback("GET /callback?code=a&state=s2 HTTP/1.1\r\n\r\n", "s1").outcome == CallbackOutcome::Failed);
    CHECK(ParseCallback("GET /callback?code=a HTTP/1.1\r\n\r\n", "s1").outcome == CallbackOutcome::Failed);
    CHECK(ParseCallback("GET /callback?code=a&state= HTTP/1.1\r\n\r\n", "").outcome == CallbackOutcome::Failed);
    CHECK(ParseCallback("GET /callback?error=access_denied&state=s1 HTTP/1.1\r\n\r\n", "s1").outcome ==
          CallbackOutcome::Denied);
    // A refusal with someone else's state is no refusal of ours.
    CHECK(ParseCallback("GET /callback?error=access_denied&state=s2 HTTP/1.1\r\n\r\n", "s1").outcome ==
          CallbackOutcome::Failed);
    CHECK(ParseCallback("GET /callback?error=invalid_request&state=s1 HTTP/1.1\r\n\r\n", "s1").outcome ==
          CallbackOutcome::Failed);
    CHECK(ParseCallback("GET /callback?state=s1 HTTP/1.1\r\n\r\n", "s1").outcome == CallbackOutcome::Failed);
}

TEST_CASE("Anything but the callback is not the callback [network][site-sign-in]")
{
    CHECK(ParseCallback("GET /favicon.ico HTTP/1.1\r\n\r\n", "s1").outcome == CallbackOutcome::NotCallback);
    CHECK(ParseCallback("GET /callbackx?state=s1&code=a HTTP/1.1\r\n\r\n", "s1").outcome ==
          CallbackOutcome::NotCallback);
    CHECK(ParseCallback("POST /callback?state=s1&code=a HTTP/1.1\r\n\r\n", "s1").outcome ==
          CallbackOutcome::NotCallback);
    CHECK(ParseCallback("\x16\x03\x01 junk", "s1").outcome == CallbackOutcome::NotCallback);
    CHECK(ParseCallback("", "s1").outcome == CallbackOutcome::NotCallback);
}

TEST_CASE("Token answers map to a sign-in, a missing ticket or the site's error [network][site-sign-in]")
{
    const TokenAnswer signedIn = ParseTokenAnswer(200, SignedIn().body);
    CHECK(signedIn.Succeeded());
    CHECK(signedIn.refreshToken == "refresh-1");
    CHECK(signedIn.name == "player");
    CHECK(signedIn.ticket == "ticket-1");
    CHECK(signedIn.error.empty());

    const TokenAnswer noTicket =
        ParseTokenAnswer(200, R"({"refresh_token":"r","name":"p","ticket":null,"ticket_error":"unavailable"})");
    CHECK(noTicket.Succeeded());
    CHECK(noTicket.ticket.empty());
    CHECK(noTicket.error == "unavailable");

    CHECK(ParseTokenAnswer(400, R"({"error":"invalid_grant"})").error == InvalidGrant);
    CHECK_FALSE(ParseTokenAnswer(400, R"({"error":"invalid_grant"})").Succeeded());
    CHECK(ParseTokenAnswer(0, "").error == NetworkError);
    CHECK(ParseTokenAnswer(503, "<html>down</html>").error == "HTTP 503");
    CHECK_FALSE(ParseTokenAnswer(200, R"({"name":"p"})").Succeeded());
    CHECK_FALSE(ParseTokenAnswer(200, "not json").Succeeded());
}

TEST_CASE("A device code polls while pending, slows down when told, and stops on the outcome [network][site-sign-in]")
{
    const auto authorization = ParseDeviceAuthorization(
        200,
        R"({"device_code":"d","user_code":"BCDF-GHJK","verification_uri":"https://x/link","interval":5,"expires_in":600})");
    REQUIRE(authorization.has_value());
    CHECK(authorization->userCode == "BCDF-GHJK");
    CHECK_FALSE(ParseDeviceAuthorization(429, "{}").has_value());

    DevicePoll poll(authorization->intervalSeconds, authorization->expiresInSeconds);
    CHECK(poll.OnAnswer({.error = "authorization_pending"}) == DevicePoll::Step::Wait);
    CHECK(poll.IntervalSeconds() == 5);
    CHECK(poll.OnAnswer({.error = "slow_down"}) == DevicePoll::Step::Wait);
    CHECK(poll.IntervalSeconds() == 10);
    CHECK(poll.OnAnswer({.error = NetworkError}) == DevicePoll::Step::Wait);
    CHECK(poll.OnAnswer({.refreshToken = "r", .ticket = "t"}) == DevicePoll::Step::SignedIn);
    CHECK(poll.OnAnswer({.error = "expired_token"}) == DevicePoll::Step::Failed);
    CHECK(poll.OnAnswer({.error = "access_denied"}) == DevicePoll::Step::Failed);
    CHECK_FALSE(poll.Expired(599));
    CHECK(poll.Expired(600));
}

TEST_CASE("A saved sign-in is kept encrypted, read back, and cleared [network][site-sign-in]")
{
    std::wstring setting;
    SavedSignInStore store(
        &TestEncrypt, &TestDecrypt, [&] { return setting; }, [&](const std::wstring& value) { setting = value; });
    CHECK_FALSE(store.Load().has_value());

    REQUIRE(store.Save({.name = "player", .refreshToken = "refresh-1"}));
    CHECK(setting.find(L"refresh") == std::wstring::npos);
    const auto loaded = store.Load();
    REQUIRE(loaded.has_value());
    CHECK(loaded->name == "player");
    CHECK(loaded->refreshToken == "refresh-1");

    SavedSignInStore elsewhere(&TestEncrypt, &OtherHostDecrypt, [&] { return setting; }, [](const std::wstring&) {});
    CHECK_FALSE(elsewhere.Load().has_value());

    store.Clear();
    CHECK(setting.empty());
    CHECK_FALSE(store.Load().has_value());
    CHECK_FALSE(store.Save({.name = "player"}));
    CHECK(setting.empty());
}

TEST_CASE("The browser's code comes back to the loopback and buys a ticket [network][site-sign-in]")
{
    FakeHttp http;
    http.answers.push_back(SignedIn());
    AllowingBrowser browser;
    SiteSignInFlow flow(http, browser, kSite, "DESKTOP-1");

    flow.SignInWithBrowser();
    const auto answer = AwaitAnswer(flow);

    REQUIRE(answer.has_value());
    CHECK(answer->ticket == "ticket-1");
    CHECK(browser.player.joinable());
    browser.player.join();
    CHECK(browser.strayPage.rfind("HTTP/1.1 404", 0) == 0);
    CHECK(browser.callbackPage.find("You can return to the game.") != std::string::npos);
    REQUIRE(http.posts.size() == 1);
    const auto& exchange = http.posts[0];
    CHECK(exchange.url == "https://mu.example.com/game-auth/token");
    CHECK(exchange.fields.at("grant_type") == "authorization_code");
    CHECK(exchange.fields.at("code") == "the+code");
    CHECK(exchange.fields.at("redirect_uri") == QueryParam(browser.opened, "redirect_uri"));
    CHECK(ChallengeOf(exchange.fields.at("code_verifier")) == QueryParam(browser.opened, "code_challenge"));
    CHECK(exchange.fields.at("client_id") == ClientId);
    CHECK(exchange.fields.at("device_name") == "DESKTOP-1");
}

TEST_CASE("A callback with another state ends the sign-in without asking the site [network][site-sign-in]")
{
    FakeHttp http;
    AllowingBrowser browser;
    browser.stateOverride = "forged";
    SiteSignInFlow flow(http, browser, kSite, "");

    flow.SignInWithBrowser();
    const auto answer = AwaitAnswer(flow);

    REQUIRE(answer.has_value());
    CHECK(answer->error == BadCallback);
    browser.player.join();
    CHECK(browser.callbackPage.find("was not signed in") != std::string::npos);
    CHECK(http.posts.empty());
}

TEST_CASE("Without the loopback the device code signs in [network][site-sign-in]")
{
    LoopbackListener taken;
    REQUIRE(taken.Listen());
    FakeHttp http;
    http.answers.push_back(
        {200,
         R"({"device_code":"dev","user_code":"BCDF-GHJK","verification_uri":"https://mu.example.com/link","interval":1,"expires_in":600})"});
    http.answers.push_back({400, R"({"error":"authorization_pending"})"});
    http.answers.push_back({400, R"({"error":"slow_down"})"});
    http.answers.push_back(SignedIn());
    AllowingBrowser unused;
    SiteSignInFlow flow(http, unused, kSite, "",
                        {.second = std::chrono::milliseconds(20), .loopbackPort = taken.Port()});

    flow.SignInWithBrowser();
    LoginPrompt prompt;
    for (int i = 0; i < 100 && prompt.code.empty(); ++i)
    {
        prompt = flow.Prompt();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    CHECK(prompt.code == "BCDF-GHJK");
    CHECK(prompt.url == "https://mu.example.com/link");
    const auto answer = AwaitAnswer(flow);

    REQUIRE(answer.has_value());
    CHECK(answer->ticket == "ticket-1");
    CHECK(unused.opened.empty());
    REQUIRE(http.posts.size() == 4);
    CHECK(http.posts[0].url == "https://mu.example.com/game-auth/device");
    CHECK(http.posts[3].fields.at("grant_type") == DeviceGrantType);
    CHECK(http.posts[3].fields.at("device_code") == "dev");
    CHECK(flow.Prompt().code.empty());
}

TEST_CASE("A device code the player refuses ends the sign-in [network][site-sign-in]")
{
    FakeHttp http;
    http.answers.push_back(
        {200,
         R"({"device_code":"dev","user_code":"BCDF-GHJK","verification_uri":"https://x/link","interval":1,"expires_in":600})"});
    http.answers.push_back({400, R"({"error":"access_denied"})"});
    NoBrowser browser;
    SiteSignInFlow flow(http, browser, kSite, "", {.second = std::chrono::milliseconds(5)});

    flow.SignInWithBrowser();
    const auto answer = AwaitAnswer(flow);

    REQUIRE(answer.has_value());
    CHECK(answer->error == "access_denied");
    CHECK(http.posts.size() == 2);
}

TEST_CASE("Cancelling stops the wait for the browser at once [network][site-sign-in]")
{
    FakeHttp http;
    class IdleBrowser final : public ISystemBrowser
    {
    public:
        bool Open(const std::string&) override
        {
            return true;
        }
    } browser;
    SiteSignInFlow flow(http, browser, kSite, "");
    flow.SignInWithBrowser();
    CHECK(flow.Prompt().inBrowser);

    const auto started = std::chrono::steady_clock::now();
    flow.Cancel();

    CHECK(std::chrono::steady_clock::now() - started < std::chrono::seconds(2));
    CHECK_FALSE(flow.TakeAnswer().has_value());
    CHECK_FALSE(flow.Prompt().inBrowser);
    CHECK(http.posts.empty());
}

TEST_CASE("A refresh swaps the token for a new one and a ticket [network][site-sign-in]")
{
    FakeHttp http;
    http.answers.push_back(SignedIn());
    http.answers.push_back({400, R"({"error":"invalid_grant"})"});
    NoBrowser browser;
    SiteSignInFlow flow(http, browser, kSite, "");

    flow.Refresh("refresh-0");
    const auto renewed = AwaitAnswer(flow);
    REQUIRE(renewed.has_value());
    CHECK(renewed->refreshToken == "refresh-1");
    CHECK(http.posts[0].fields.at("grant_type") == "refresh_token");
    CHECK(http.posts[0].fields.at("refresh_token") == "refresh-0");

    flow.Refresh("refresh-0");
    const auto revoked = AwaitAnswer(flow);
    REQUIRE(revoked.has_value());
    CHECK(revoked->error == InvalidGrant);
}
