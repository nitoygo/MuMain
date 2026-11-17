#include "stdafx.h"

#include "Network/Auth/LoginRequest.h"

namespace Network::Auth
{
std::wstring AccountName(const LoginRequest& request)
{
    if (const auto* password = std::get_if<PasswordLogin>(&request))
        return password->username;
    return {};
}

void PrepareRelogin(const LoginRequest& previous, ILoginTicketSource& source, LoginReadyCallback ready,
                    LoginFailedCallback failed)
{
    if (std::holds_alternative<PasswordLogin>(previous))
    {
        ready(previous);
        return;
    }

    source.Renew(
        [ready = std::move(ready), failed = std::move(failed)](const LoginTicketResult& result)
        {
            if (!result.Succeeded())
            {
                failed(result.error);
                return;
            }
            ready(TicketLogin{result.token});
        });
}
} // namespace Network::Auth
