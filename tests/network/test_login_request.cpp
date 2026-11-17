#include "stdafx.h"

#include "doctest.h"

#include "../data/UserConfigFile.h"
#include "Data/GameConfig/GameConfig.h"
#include "Network/Auth/LoginPackets.h"
#include "Network/Auth/LoginRequest.h"

#include <optional>
#include <string>

using namespace Network::Auth;

namespace
{
class RecordingPackets final : public ILoginPackets
{
public:
    void SendLogin(const wchar_t* username, const wchar_t* password) override
    {
        passwordLogins.push_back({username, password});
    }

    void SendLoginWithTicket(const char* token) override
    {
        tickets.emplace_back(token);
    }

    std::vector<PasswordLogin> passwordLogins;
    std::vector<std::string> tickets;
};

class FakeTicketSource final : public ILoginTicketSource
{
public:
    std::vector<LoginProvider> Providers() const override
    {
        return providers;
    }

    void Acquire(const std::string& providerId, LoginTicketCallback done) override
    {
        acquiredFrom.push_back(providerId);
        done(next);
    }

    void Renew(LoginTicketCallback done) override
    {
        ++renewals;
        done(next);
    }

    std::vector<LoginProvider> providers;
    LoginTicketResult next;
    std::vector<std::string> acquiredFrom;
    int renewals = 0;
};

struct Relogin
{
    std::optional<LoginRequest> ready;
    std::optional<std::string> failure;

    void Prepare(const LoginRequest& previous, ILoginTicketSource& source)
    {
        PrepareRelogin(
            previous, source, [this](const LoginRequest& request) { ready = request; },
            [this](const std::string& error) { failure = error; });
    }
};

constexpr const char* kUsedToken = "used-ticket-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
constexpr const char* kFreshToken = "fresh-ticket-BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
} // namespace

TEST_CASE("A password request sends the password login packet [network][login]")
{
    RecordingPackets packets;
    SendLoginRequest(PasswordLogin{L"ancient", L"secret"}, packets);

    REQUIRE(packets.passwordLogins.size() == 1);
    CHECK(packets.passwordLogins[0].username == L"ancient");
    CHECK(packets.passwordLogins[0].password == L"secret");
    CHECK(packets.tickets.empty());
}

TEST_CASE("A ticket request sends the ticket login packet [network][login]")
{
    RecordingPackets packets;
    SendLoginRequest(TicketLogin{kFreshToken}, packets);

    CHECK(packets.passwordLogins.empty());
    REQUIRE(packets.tickets.size() == 1);
    CHECK(packets.tickets[0] == kFreshToken);
}

TEST_CASE("Only a password request names an account [network][login]")
{
    CHECK(AccountName(PasswordLogin{L"ancient", L"secret"}) == L"ancient");
    CHECK(AccountName(TicketLogin{kFreshToken}).empty());
}

TEST_CASE("A relogin with a password sends it again without the ticket source [network][login]")
{
    FakeTicketSource source;
    Relogin relogin;
    relogin.Prepare(PasswordLogin{L"ancient", L"secret"}, source);

    CHECK(source.renewals == 0);
    REQUIRE(relogin.ready.has_value());
    const auto* password = std::get_if<PasswordLogin>(&*relogin.ready);
    REQUIRE(password != nullptr);
    CHECK(password->username == L"ancient");
    CHECK(password->password == L"secret");
}

TEST_CASE("A relogin with a ticket renews it and never sends the used one [network][login]")
{
    FakeTicketSource source;
    source.next.token = kFreshToken;
    Relogin relogin;
    relogin.Prepare(TicketLogin{kUsedToken}, source);

    CHECK(source.renewals == 1);
    CHECK_FALSE(relogin.failure.has_value());
    REQUIRE(relogin.ready.has_value());
    const auto* ticket = std::get_if<TicketLogin>(&*relogin.ready);
    REQUIRE(ticket != nullptr);
    CHECK(ticket->token == kFreshToken);
}

TEST_CASE("A relogin fails when the ticket source has no fresh ticket [network][login]")
{
    FakeTicketSource source;
    source.next.error = "signed out";
    Relogin relogin;
    relogin.Prepare(TicketLogin{kUsedToken}, source);

    CHECK(source.renewals == 1);
    CHECK_FALSE(relogin.ready.has_value());
    REQUIRE(relogin.failure.has_value());
    CHECK(*relogin.failure == "signed out");
}

TEST_CASE("The default ticket source offers no providers and gives no tickets [network][login]")
{
    // Without a SiteUrl or a saved sign-in, whatever the developer's own config.ini says.
    const UserConfigFile config;
    config.Write("[CONNECTION SETTINGS]\nSiteUrl=\n");
    GameConfig::GetInstance().Load();

    ILoginTicketSource& source = LoginTicketSource();
    CHECK(source.Providers().empty());

    std::optional<LoginTicketResult> acquired;
    source.Acquire("discord", [&](const LoginTicketResult& result) { acquired = result; });
    REQUIRE(acquired.has_value());
    CHECK_FALSE(acquired->Succeeded());

    std::optional<LoginTicketResult> renewed;
    source.Renew([&](const LoginTicketResult& result) { renewed = result; });
    REQUIRE(renewed.has_value());
    CHECK_FALSE(renewed->Succeeded());
}
