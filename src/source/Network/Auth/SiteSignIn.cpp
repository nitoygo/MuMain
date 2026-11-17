#include "stdafx.h"

#include "Network/Auth/SiteSignIn.h"

#include "Core/Platform/PlatformCrypto.h"

#include "json.hpp"

#include <array>
#include <cctype>

namespace Network::Auth::SiteSignIn
{
namespace
{
constexpr std::size_t kVerifierBytes = 32;
constexpr std::size_t kStateBytes = 16;
constexpr int kHttpOk = 200;

int HexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

std::string UrlDecode(std::string_view value)
{
    std::string decoded;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        const bool escaped =
            value[i] == '%' && i + 2 < value.size() && HexValue(value[i + 1]) >= 0 && HexValue(value[i + 2]) >= 0;
        if (escaped)
        {
            decoded += static_cast<char>(HexValue(value[i + 1]) * 16 + HexValue(value[i + 2]));
            i += 2;
        }
        else
        {
            decoded += value[i] == '+' ? ' ' : value[i];
        }
    }
    return decoded;
}

// One parameter of a query string, decoded; nullopt when absent.
std::optional<std::string> QueryValue(std::string_view query, std::string_view name)
{
    while (!query.empty())
    {
        const std::size_t end = query.find('&');
        const std::string_view pair = query.substr(0, end);
        const std::size_t equals = pair.find('=');
        if (pair.substr(0, equals) == name)
            return UrlDecode(equals == std::string_view::npos ? std::string_view{} : pair.substr(equals + 1));
        query = end == std::string_view::npos ? std::string_view{} : query.substr(end + 1);
    }
    return std::nullopt;
}

std::string StringOf(const nlohmann::json& object, const char* key)
{
    const auto found = object.find(key);
    return found != object.end() && found->is_string() ? found->get<std::string>() : std::string{};
}

int IntOf(const nlohmann::json& object, const char* key, int fallback)
{
    const auto found = object.find(key);
    return found != object.end() && found->is_number_integer() ? found->get<int>() : fallback;
}

std::string RandomBase64Url(std::size_t bytes)
{
    std::array<std::uint8_t, kVerifierBytes> random = {};
    if (bytes > random.size() || !mu_random_bytes(random.data(), bytes))
        return {};
    return Base64Url(random.data(), bytes);
}
} // namespace

std::string Base64Url(const std::uint8_t* data, std::size_t size)
{
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string encoded;
    std::uint32_t buffer = 0;
    int bits = 0;
    for (std::size_t i = 0; i < size; ++i)
    {
        buffer = (buffer << 8) | data[i];
        bits += 8;
        while (bits >= 6)
        {
            bits -= 6;
            encoded += kAlphabet[(buffer >> bits) & 0x3F];
        }
    }
    if (bits > 0)
        encoded += kAlphabet[(buffer << (6 - bits)) & 0x3F];
    return encoded;
}

std::string ChallengeOf(std::string_view verifier)
{
    std::array<std::uint8_t, MU_SHA256_SIZE> digest = {};
    if (!mu_sha256(verifier.data(), verifier.size(), digest.data()))
        return {};
    return Base64Url(digest.data(), digest.size());
}

std::optional<Pkce> NewPkce()
{
    Pkce pkce{.verifier = RandomBase64Url(kVerifierBytes)};
    pkce.challenge = ChallengeOf(pkce.verifier);
    if (pkce.verifier.empty() || pkce.challenge.empty())
        return std::nullopt;
    return pkce;
}

std::string NewState()
{
    return RandomBase64Url(kStateBytes);
}

std::string UrlEncode(std::string_view value)
{
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string encoded;
    for (const char c : value)
    {
        const auto byte = static_cast<unsigned char>(c);
        const bool unreserved = std::isalnum(byte) != 0 || c == '-' || c == '.' || c == '_' || c == '~';
        if (unreserved)
        {
            encoded += c;
            continue;
        }
        encoded += '%';
        encoded += kHex[byte >> 4];
        encoded += kHex[byte & 0x0F];
    }
    return encoded;
}

std::string LoopbackRedirect(unsigned short port)
{
    return "http://127.0.0.1:" + std::to_string(port) + "/callback";
}

std::string AuthorizeUrl(std::string_view site, std::string_view redirectUri, std::string_view state,
                         std::string_view challenge)
{
    return std::string(site) + AuthorizePath + "?client_id=" + ClientId +
           "&response_type=code&redirect_uri=" + UrlEncode(redirectUri) + "&state=" + UrlEncode(state) +
           "&code_challenge=" + UrlEncode(challenge) + "&code_challenge_method=S256";
}

Callback ParseCallback(std::string_view request, std::string_view expectedState)
{
    const std::string_view line = request.substr(0, request.find("\r\n"));
    constexpr std::string_view kPrefix = "GET /callback";
    if (line.substr(0, kPrefix.size()) != kPrefix)
        return {.outcome = CallbackOutcome::NotCallback};

    const std::string_view rest = line.substr(kPrefix.size());
    const std::string_view target = rest.substr(0, rest.find(' '));
    if (!target.empty() && target.front() != '?')
        return {.outcome = CallbackOutcome::NotCallback};

    const std::string_view query = target.empty() ? target : target.substr(1);
    // Another page or app can reach the listener too; only our own state proves the browser came from our request.
    if (QueryValue(query, "state") != std::string(expectedState) || expectedState.empty())
        return {.outcome = CallbackOutcome::Failed};
    if (const auto error = QueryValue(query, "error"))
        return {.outcome = *error == "access_denied" ? CallbackOutcome::Denied : CallbackOutcome::Failed};
    const auto code = QueryValue(query, "code");
    if (!code || code->empty())
        return {.outcome = CallbackOutcome::Failed};
    return {.outcome = CallbackOutcome::Code, .code = *code};
}

TokenAnswer ParseTokenAnswer(int status, std::string_view body)
{
    if (status == 0)
        return {.error = NetworkError};

    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (!json.is_object())
        return {.error = "HTTP " + std::to_string(status)};

    if (status != kHttpOk)
    {
        const std::string error = StringOf(json, "error");
        return {.error = error.empty() ? "HTTP " + std::to_string(status) : error};
    }

    TokenAnswer answer{.refreshToken = StringOf(json, "refresh_token"),
                       .name = StringOf(json, "name"),
                       .ticket = StringOf(json, "ticket")};
    if (answer.refreshToken.empty())
        answer.error = "no refresh token in the answer";
    else if (answer.ticket.empty())
        answer.error = StringOf(json, "ticket_error").empty() ? "no ticket" : StringOf(json, "ticket_error");
    return answer;
}

std::optional<DeviceAuthorization> ParseDeviceAuthorization(int status, std::string_view body)
{
    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (status != kHttpOk || !json.is_object())
        return std::nullopt;

    DeviceAuthorization authorization{.deviceCode = StringOf(json, "device_code"),
                                      .userCode = StringOf(json, "user_code"),
                                      .verificationUri = StringOf(json, "verification_uri")};
    authorization.intervalSeconds = IntOf(json, "interval", authorization.intervalSeconds);
    authorization.expiresInSeconds = IntOf(json, "expires_in", authorization.expiresInSeconds);
    if (authorization.deviceCode.empty() || authorization.userCode.empty() || authorization.verificationUri.empty())
        return std::nullopt;
    return authorization;
}

std::string SiteHost(std::string_view site)
{
    const std::size_t scheme = site.find("://");
    const std::string_view rest = scheme == std::string_view::npos ? site : site.substr(scheme + 3);
    return std::string(rest.substr(0, rest.find_first_of("/:?#")));
}
} // namespace Network::Auth::SiteSignIn
