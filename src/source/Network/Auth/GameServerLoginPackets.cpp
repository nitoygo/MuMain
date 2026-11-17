#include "stdafx.h"

#include "Network/Auth/LoginPackets.h"

#include "Network/Server/WSclient.h"

extern BYTE Version[SIZE_PROTOCOLVERSION];
extern BYTE Serial[SIZE_PROTOCOLSERIAL + 1];

namespace Network::Auth
{
namespace
{
class GameServerPackets final : public ILoginPackets
{
public:
    void SendLogin(const wchar_t* username, const wchar_t* password) override
    {
        SocketClient->ToGameServer()->SendLogin(username, password, Version, Serial);
    }

    void SendLoginWithTicket(const char* token) override
    {
        SocketClient->ToGameServer()->SendLoginWithTicket(token, Version, Serial);
    }
};
} // namespace

ILoginPackets& GameServerLoginPackets()
{
    static GameServerPackets packets;
    return packets;
}
} // namespace Network::Auth
