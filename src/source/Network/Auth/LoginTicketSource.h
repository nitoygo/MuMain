#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Network::Auth
{
// A sign-in provider the player can pick, e.g. {"discord", "Discord"}.
struct LoginProvider
{
    std::string id;
    std::string displayName;
    // The player already chose it outside the game (e.g. pressed Play on the website while signed in): the login
    // window continues with it once, without another click.
    bool autoContinue = false;
};

// A single-use login ticket, or why there is none.
struct LoginTicketResult
{
    std::string token;
    std::string error;

    bool Succeeded() const
    {
        return !token.empty();
    }
};

using LoginTicketCallback = std::function<void(const LoginTicketResult&)>;

// Where the client gets login tickets: the player signs in with a provider outside the game (a website, a browser),
// and the source hands over a ticket the game server redeems once. The client never sees a provider's own token.
// One adapter per target; callbacks run on the main thread, possibly before Acquire()/Renew() return.
class ILoginTicketSource
{
public:
    virtual ~ILoginTicketSource() = default;

    // The providers to offer a sign-in button for; none hides the buttons.
    virtual std::vector<LoginProvider> Providers() const = 0;

    // Signs in with the provider, which may take the player to a browser, and delivers a ticket.
    virtual void Acquire(const std::string& providerId, LoginTicketCallback done) = 0;

    // A fresh ticket for the account signed in last, to log back in after a disconnect.
    virtual void Renew(LoginTicketCallback done) = 0;
};

// The target's source, from the adapter its target file names.
ILoginTicketSource& LoginTicketSource();
} // namespace Network::Auth
