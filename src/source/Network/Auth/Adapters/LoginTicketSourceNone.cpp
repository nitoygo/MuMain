// For a target without sign-in providers: the login window shows no provider buttons.
#include "Network/Auth/LoginTicketSource.h"

namespace Network::Auth
{
namespace
{
constexpr const char* kNoProviders = "This client has no sign-in providers.";

class NoLoginTicketSource final : public ILoginTicketSource
{
public:
    std::vector<LoginProvider> Providers() const override
    {
        return {};
    }

    void Acquire(const std::string&, LoginTicketCallback done) override
    {
        done({.error = kNoProviders});
    }

    void Renew(LoginTicketCallback done) override
    {
        done({.error = kNoProviders});
    }
};
} // namespace

ILoginTicketSource& LoginTicketSource()
{
    static NoLoginTicketSource source;
    return source;
}
} // namespace Network::Auth
