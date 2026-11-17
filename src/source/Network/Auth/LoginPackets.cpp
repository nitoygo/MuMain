#include "stdafx.h"

#include "Network/Auth/LoginPackets.h"

namespace Network::Auth
{
void SendLoginRequest(const LoginRequest& request, ILoginPackets& packets)
{
    if (const auto* password = std::get_if<PasswordLogin>(&request))
    {
        packets.SendLogin(password->username.c_str(), password->password.c_str());
        return;
    }
    packets.SendLoginWithTicket(std::get<TicketLogin>(request).token.c_str());
}
} // namespace Network::Auth
