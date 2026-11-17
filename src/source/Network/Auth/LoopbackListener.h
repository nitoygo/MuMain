#pragma once

#include "Core/Platform/WinSock.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace Network::Auth
{
// The game's end of RFC 8252's loopback redirect: listens on 127.0.0.1 only, takes one browser request at a time and
// answers it. Blocking calls bounded by short waits, for a worker thread that must stop when told to.
class LoopbackListener
{
public:
    // The most of a request that is read: its request line and headers.
    static constexpr std::size_t MaxRequestBytes = 8 * 1024;

    LoopbackListener() = default;
    ~LoopbackListener();

    LoopbackListener(const LoopbackListener&) = delete;
    LoopbackListener& operator=(const LoopbackListener&) = delete;

    // Binds 127.0.0.1 on the port (0: one the system picks). False when that isn't possible.
    bool Listen(unsigned short port = 0);

    unsigned short Port() const
    {
        return m_port;
    }

    // The next request's head, waiting until the deadline; nullopt on timeout, cancel or a socket failure.
    std::optional<std::string> NextRequest(const std::atomic_bool& cancel,
                                           std::chrono::steady_clock::time_point deadline);

    // Answers the request NextRequest() returned with a small HTML page, and closes that connection.
    void Answer(int status, std::string_view html);

    void Close();

private:
    // Waits until the socket is readable, at most the wait; false on timeout or failure.
    static bool WaitReadable(SOCKET socket, std::chrono::milliseconds wait);
    std::optional<std::string> ReadHead(SOCKET connection) const;
    void CloseConnection();

    SOCKET m_listener = INVALID_SOCKET;
    SOCKET m_connection = INVALID_SOCKET;
    unsigned short m_port = 0;
    bool m_started = false;
};
} // namespace Network::Auth
