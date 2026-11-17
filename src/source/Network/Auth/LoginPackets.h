#pragma once

#include "Network/Auth/LoginRequest.h"

namespace Network::Auth
{
// The login packets a game server connection sends.
class ILoginPackets
{
public:
    virtual ~ILoginPackets() = default;

    virtual void SendLogin(const wchar_t* username, const wchar_t* password) = 0;
    virtual void SendLoginWithTicket(const char* token) = 0;
};

// Sends the packet the request stands for.
void SendLoginRequest(const LoginRequest& request, ILoginPackets& packets);

// The packets over the client's game server connection.
ILoginPackets& GameServerLoginPackets();
} // namespace Network::Auth
