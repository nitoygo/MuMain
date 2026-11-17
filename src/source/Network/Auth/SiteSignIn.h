#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// The game's sign-in at its website (/game-auth): RFC 8252's system browser with a loopback redirect and PKCE
// (RFC 7636), or RFC 8628's device code. What goes over the wire and how answers read; SiteSignInFlow runs it.
namespace Network::Auth::SiteSignIn
{
// The game's public client id at the site; a native app keeps no secret.
inline constexpr const char* ClientId = "mu-client";
inline constexpr const char* AuthorizePath = "/game-auth/authorize";
inline constexpr const char* TokenPath = "/game-auth/token";
inline constexpr const char* DevicePath = "/game-auth/device";
inline constexpr const char* DeviceGrantType = "urn:ietf:params:oauth:grant-type:device_code";

// Errors besides the site's own (RFC 6749 5.2, RFC 8628 3.5); for the log, the player gets the login window's message.
inline constexpr const char* NetworkError = "network";
inline constexpr const char* NoRandom = "no-random";
inline constexpr const char* Cancelled = "cancelled";
inline constexpr const char* TimedOut = "timeout";
inline constexpr const char* BadCallback = "bad-callback";
// The site no longer takes the refresh token (revoked, replaced, idle too long): the player signs in again.
inline constexpr const char* InvalidGrant = "invalid_grant";

struct Pkce
{
    std::string verifier;
    std::string challenge;
};

std::string Base64Url(const std::uint8_t* data, std::size_t size);
// BASE64URL(SHA256(verifier)), the S256 challenge; empty when the target can't hash.
std::string ChallengeOf(std::string_view verifier);
// A verifier from 32 random bytes (43 characters) and its challenge; nullopt without a random source.
std::optional<Pkce> NewPkce();
// 128 random bits for the state parameter; empty without a random source.
std::string NewState();

std::string UrlEncode(std::string_view value);
std::string LoopbackRedirect(unsigned short port);
std::string AuthorizeUrl(std::string_view site, std::string_view redirectUri, std::string_view state,
                         std::string_view challenge);

enum class CallbackOutcome
{
    Code,
    Denied,
    Failed,
    // Something else asked the listener (e.g. /favicon.ico); it answers 404 and keeps waiting.
    NotCallback,
};

struct Callback
{
    CallbackOutcome outcome = CallbackOutcome::Failed;
    std::string code;
};

// The browser's request to the loopback, from its request line: "GET /callback?code=…&state=… HTTP/1.1".
Callback ParseCallback(std::string_view request, std::string_view expectedState);

// An answer of POST /game-auth/token. A sign-in succeeded when it has a refresh token; the ticket may still be
// missing when the game server couldn't issue one, and error then says why.
struct TokenAnswer
{
    std::string refreshToken;
    std::string name;
    std::string ticket;
    std::string error;

    bool Succeeded() const
    {
        return !refreshToken.empty();
    }
};

// status is the HTTP status, 0 when the request got none.
TokenAnswer ParseTokenAnswer(int status, std::string_view body);

struct DeviceAuthorization
{
    std::string deviceCode;
    std::string userCode;
    std::string verificationUri;
    int intervalSeconds = 5;
    int expiresInSeconds = 600;
};

std::optional<DeviceAuthorization> ParseDeviceAuthorization(int status, std::string_view body);

// The host of a site URL, as the sign-in button names it: "https://mu.example.com/" is "mu.example.com".
std::string SiteHost(std::string_view site);
} // namespace Network::Auth::SiteSignIn
