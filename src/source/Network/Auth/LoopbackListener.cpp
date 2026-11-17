#include "stdafx.h"

#include "Network/Auth/LoopbackListener.h"

#include <cstring>

namespace Network::Auth
{
namespace
{
// How long one wait lasts before the cancel flag is looked at again.
constexpr std::chrono::milliseconds kWaitSlice{200};
// A browser sends its request right after connecting; one that doesn't is dropped.
constexpr std::chrono::milliseconds kRequestWait{5000};
constexpr std::size_t kReadChunk = 1024;
#ifdef MSG_NOSIGNAL
constexpr int kSendFlags = MSG_NOSIGNAL; // a closed browser tab must not end the game with SIGPIPE
#else
constexpr int kSendFlags = 0;
#endif

const char* StatusText(int status)
{
    return status == 200 ? "OK" : status == 404 ? "Not Found" : "Bad Request";
}
} // namespace

LoopbackListener::~LoopbackListener()
{
    Close();
}

bool LoopbackListener::Listen(unsigned short port)
{
    Close();
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return false;
    m_started = true;

    m_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listener == INVALID_SOCKET)
        return false;

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t size = sizeof(address);
    const bool listening = bind(m_listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0 &&
                           listen(m_listener, 1) == 0 &&
                           getsockname(m_listener, reinterpret_cast<sockaddr*>(&address), &size) == 0;
    if (!listening)
    {
        Close();
        return false;
    }
    m_port = ntohs(address.sin_port);
    return true;
}

std::optional<std::string> LoopbackListener::NextRequest(const std::atomic_bool& cancel,
                                                         std::chrono::steady_clock::time_point deadline)
{
    CloseConnection();
    while (m_listener != INVALID_SOCKET && !cancel && std::chrono::steady_clock::now() < deadline)
    {
        if (!WaitReadable(m_listener, kWaitSlice))
            continue;
        const SOCKET connection = accept(m_listener, nullptr, nullptr);
        if (connection == INVALID_SOCKET)
            continue;
        m_connection = connection;
        if (auto head = ReadHead(connection))
            return head;
        CloseConnection();
    }
    return std::nullopt;
}

void LoopbackListener::Answer(int status, std::string_view html)
{
    if (m_connection == INVALID_SOCKET)
        return;
    const std::string response =
        "HTTP/1.1 " + std::to_string(status) + " " + StatusText(status) +
        "\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + std::to_string(html.size()) +
        "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" + std::string(html);
    std::size_t sent = 0;
    while (sent < response.size())
    {
        const int written =
            send(m_connection, response.data() + sent, static_cast<int>(response.size() - sent), kSendFlags);
        if (written <= 0)
            break;
        sent += static_cast<std::size_t>(written);
    }
    CloseConnection();
}

void LoopbackListener::Close()
{
    CloseConnection();
    if (m_listener != INVALID_SOCKET)
    {
        closesocket(m_listener);
        m_listener = INVALID_SOCKET;
    }
    if (m_started)
    {
        WSACleanup();
        m_started = false;
    }
}

bool LoopbackListener::WaitReadable(SOCKET socket, std::chrono::milliseconds wait)
{
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(socket, &readable);
    timeval timeout{};
    timeout.tv_sec = static_cast<long>(wait.count() / 1000);
    timeout.tv_usec = static_cast<long>((wait.count() % 1000) * 1000);
    return select(static_cast<int>(socket) + 1, &readable, nullptr, nullptr, &timeout) > 0;
}

std::optional<std::string> LoopbackListener::ReadHead(SOCKET connection) const
{
    std::string head;
    const auto deadline = std::chrono::steady_clock::now() + kRequestWait;
    char chunk[kReadChunk];
    while (head.find("\r\n\r\n") == std::string::npos && head.size() < MaxRequestBytes)
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return std::nullopt;
        if (!WaitReadable(connection, kWaitSlice))
            continue;
        const int read = recv(connection, chunk, static_cast<int>(sizeof(chunk)), 0);
        if (read <= 0)
            return std::nullopt;
        head.append(chunk, static_cast<std::size_t>(read));
    }
    return head;
}

void LoopbackListener::CloseConnection()
{
    if (m_connection != INVALID_SOCKET)
    {
        closesocket(m_connection);
        m_connection = INVALID_SOCKET;
    }
}
} // namespace Network::Auth
