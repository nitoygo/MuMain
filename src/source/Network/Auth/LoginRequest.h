#pragma once

#include <functional>
#include <string>
#include <variant>

#include "Network/Auth/LoginTicketSource.h"

namespace Network::Auth
{
// A login with the account's name and password, as the player typed them.
struct PasswordLogin
{
    std::wstring username;
    std::wstring password;
};

// A login with a single-use ticket from the target's ticket source.
struct TicketLogin
{
    std::string token;
};

using LoginRequest = std::variant<PasswordLogin, TicketLogin>;

// The account name the client shows for the request; a ticket names none.
std::wstring AccountName(const LoginRequest& request);

using LoginReadyCallback = std::function<void(const LoginRequest&)>;
using LoginFailedCallback = std::function<void(const std::string& error)>;

// The request to log back in with after a disconnect. A password login goes again as it was. A ticket was used up
// by the first login, so the source renews it rather than the old one being sent again.
void PrepareRelogin(const LoginRequest& previous, ILoginTicketSource& source, LoginReadyCallback ready,
                    LoginFailedCallback failed);
} // namespace Network::Auth
