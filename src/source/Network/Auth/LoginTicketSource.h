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

// What the player has to do outside the game while Acquire() waits on them.
struct LoginPrompt
{
    // Finishing the sign-in in the browser the source opened.
    bool inBrowser = false;
    // Or entering this code at this address, on any device (e.g. "BCDF-GHJK" at "https://example.com/link").
    std::string url;
    std::string code;
};

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

    // Stops a running Acquire(), e.g. when the login window closes; its callback is not called.
    virtual void Cancel() {}

    // What the running Acquire() waits for the player to do; empty when nothing.
    virtual LoginPrompt Prompt() const
    {
        return {};
    }
};

// The target's source, from the adapter its target file names.
ILoginTicketSource& LoginTicketSource();
} // namespace Network::Auth
