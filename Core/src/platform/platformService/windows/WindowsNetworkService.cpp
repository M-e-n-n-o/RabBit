#ifdef RB_PLATFORM_WINDOWS
#include "RabBitCommon.h"
#include "WindowsNetworkService.h"

#include "app/Application.h"

#include <WS2tcpip.h>
#include <iphlpapi.h>

namespace RB
{
    // These get added to before and after every message so that buffering can be done
    static const char START_MESSAGE[] = "strwin";
    static const char END_MESSAGE[] = "endwin";

    WindowsNetworkService::WindowsNetworkService()
        : m_HasNetworkConnection(false)
        , m_IsHost(false)
        , m_TcpListenSocket(INVALID_SOCKET)
        , m_HostConnection(INVALID_SOCKET)
        , m_FrameAllocator(nullptr)
    {
        WSAData wsa_data;

        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Windows network startup failed with code: %d", result);
            m_IsValid = false;
            return;
        }

        m_FrameAllocator = Application::GetInstance()->GetAllocator();

        m_IsValid = true;
    }

    WindowsNetworkService::~WindowsNetworkService()
    {
        if (!m_IsValid)
            return;

        LeaveLobby();
        WSACleanup();
    }

    void WindowsNetworkService::Update()
    {
        if (m_HasNetworkConnection && m_IsHost)
        {
            SOCKET client = accept(m_TcpListenSocket, NULL, NULL);
            if (client == INVALID_SOCKET)
            {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK)
                {
                    RB_LOG_ERROR(LOGTAG_MAIN, "Failed to accept client: %d", error);
                }
                return;
            }

            RB_LOG(LOGTAG_MAIN, "Accepted new client");

            DisableBlocking(client);

            m_ClientSockets.push_back(client);
        }
    }

    void WindowsNetworkService::CreateLobby(LobbyType lobby_type, uint32_t max_members)
    {
        if (m_HasNetworkConnection)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Make sure to leave the current lobby before creating a new one");
            return;
        }

        // Currently just open only a TCP port for simplicity.
        // If this should ever be made more shippable we need UDP (as well)!

        addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_flags      = AI_PASSIVE;
        hints.ai_family     = AF_INET6;
        hints.ai_socktype   = SOCK_STREAM;
        hints.ai_protocol   = IPPROTO_TCP;

        ADDRINFO* result = nullptr;
        int code = getaddrinfo(NULL, std::to_string(c_Port).c_str(), &hints, &result);
        if (code != 0)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to get TCP address info to create a lobby, error: %d", code);
            return;
        }

        m_TcpListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (m_TcpListenSocket == INVALID_SOCKET)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to open TCP socket, error: %ld", WSAGetLastError());
            freeaddrinfo(result);
            return;
        }

        // Disable IPv6 only mode to allow IPv4 fallback traffic
        int ipv6_only = 0;
        setsockopt(m_TcpListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6_only, sizeof(ipv6_only));

        code = bind(m_TcpListenSocket, result->ai_addr, (int)result->ai_addrlen);
        freeaddrinfo(result);

        if (code == SOCKET_ERROR)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to bind TCP socket, error: %ld", WSAGetLastError());
            closesocket(m_TcpListenSocket);
            return;
        }

        code = listen(m_TcpListenSocket, SOMAXCONN);
        if (code == SOCKET_ERROR)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to start listening on TCP socket, error: %ld", WSAGetLastError());
            closesocket(m_TcpListenSocket);
            return;
        }

        DisableBlocking(m_TcpListenSocket);

        RB_LOG(LOGTAG_MAIN, "Created lobby");
        m_IsHost = true;
        m_HasNetworkConnection = true;
    }

    void WindowsNetworkService::JoinLobby(uint64_t lobby_id)
    {
        if (m_HasNetworkConnection)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Make sure to leave the current lobby before joining a new one");
            return;
        }

        RB_LOG(LOGTAG_MAIN, "Trying to connect to lobby with address: %s", (char*)lobby_id);

        // Currently just open only a TCP port for simplicity.
        // If this should ever be made more shippable we need UDP (as well)!

        addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_flags      = AI_PASSIVE;
        hints.ai_family     = AF_UNSPEC;
        hints.ai_socktype   = SOCK_STREAM;
        hints.ai_protocol   = IPPROTO_TCP;

        ADDRINFO* result = nullptr;
        int code = getaddrinfo((char*)lobby_id, std::to_string(c_Port).c_str(), &hints, &result);
        if (code != 0)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to get TCP address info to join a lobby, error: %d", code);
            return;
        }

        m_HostConnection = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (m_HostConnection == INVALID_SOCKET)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to open TCP socket, error: %ld", WSAGetLastError());
            freeaddrinfo(result);
            return;
        }

        code = connect(m_HostConnection, result->ai_addr, (int)result->ai_addrlen);
        freeaddrinfo(result);

        if (code == SOCKET_ERROR || m_HostConnection == INVALID_SOCKET)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to connect to lobby");
            closesocket(m_HostConnection);
            return;
        }

        DisableBlocking(m_HostConnection);

        RB_LOG(LOGTAG_MAIN, "Joined lobby");
        m_IsHost = false;
        m_HasNetworkConnection = true;
    }

    void WindowsNetworkService::LeaveLobby()
    {
        if (!m_HasNetworkConnection)
            return;

        RB_LOG(LOGTAG_MAIN, "Leaving lobby");

        if (m_IsHost)
        {
            for (SOCKET s : m_ClientSockets)
                closesocket(s);
            m_ClientSockets.clear();

            closesocket(m_TcpListenSocket);
        }
        else
        {
            closesocket(m_HostConnection);
        }

        m_IsHost = false;
        m_HasNetworkConnection = false;
        m_TcpListenSocket = INVALID_SOCKET;
        m_HostConnection = INVALID_SOCKET;
    }

    bool WindowsNetworkService::IsConnected() const
    {
        return m_HasNetworkConnection;
    }

    bool WindowsNetworkService::IsHost() const
    {
        return m_HasNetworkConnection && m_IsHost;
    }

    uint64_t WindowsNetworkService::GetLobbyID() const
    {
        // TODO: Base the ID based on a handshake after being accepted
        return 0;
    }

    uint64_t WindowsNetworkService::GetPlayerID() const
    {
        // TODO: Base the ID based on a handshake after being accepted
        return m_IsHost ? 0 : 1;
    }

    void WindowsNetworkService::Broadcast(const DataPackage* package, bool reliable)
    {
        if (!m_IsHost || !m_HasNetworkConnection)
            return;

        for (auto itr = m_ClientSockets.begin(); itr != m_ClientSockets.end(); ++itr)
        {
            int result = send(*itr, START_MESSAGE, sizeof(START_MESSAGE), 0);
            if (result == SOCKET_ERROR) RB_LOG_WARN(LOGTAG_MAIN, "Failed to broadcast data to a client");

            result = send(*itr, (const char*)package->data, package->size, 0);
            if (result == SOCKET_ERROR) RB_LOG_WARN(LOGTAG_MAIN, "Failed to broadcast data to a client");

            result = send(*itr, END_MESSAGE, sizeof(END_MESSAGE), 0);
            if (result == SOCKET_ERROR) RB_LOG_WARN(LOGTAG_MAIN, "Failed to broadcast data to a client");
        }
    }

    void WindowsNetworkService::SendToHost(const DataPackage* package, bool reliable)
    {
        if (m_IsHost || !m_HasNetworkConnection)
            return;

        int result = send(m_HostConnection, START_MESSAGE, sizeof(START_MESSAGE), 0);
        if (result == SOCKET_ERROR) RB_LOG_WARN(LOGTAG_MAIN, "Failed to send data to host");

        result = send(m_HostConnection, (const char*)package->data, package->size, 0);
        if (result == SOCKET_ERROR) RB_LOG_WARN(LOGTAG_MAIN, "Failed to send data to host");

        result = send(m_HostConnection, END_MESSAGE, sizeof(END_MESSAGE), 0);
        if (result == SOCKET_ERROR) RB_LOG_WARN(LOGTAG_MAIN, "Failed to send data to host");
    }

    DataPackage* WindowsNetworkService::GetReceivedPackages(uint32_t& out_total_packages)
    {
        if (!m_HasNetworkConnection)
        {
            out_total_packages = 0;
            return nullptr;
        }

        const uint32_t max_recv_chunk = 1024;
        List<DataPackage> found_packages;

        if (m_IsHost)
        {
            if (m_ClientSockets.empty())
            {
                out_total_packages = 0;
                return nullptr;
            }

            for (auto itr = m_ClientSockets.begin(); itr != m_ClientSockets.end();)
            {
                SOCKET& socket = *itr;
                char temp[max_recv_chunk];
                int bytes_received = recv(socket, temp, sizeof(temp), 0);

                if (bytes_received > 0)
                {
                    RecvBuffer& buf = m_ClientRecvBuffers[socket];
                    AppendToRecvBuffer(buf, temp, bytes_received);
                    ExtractPackages(buf, found_packages);
                    ++itr;
                }
                else if (bytes_received == 0)
                {
                    RB_LOG(LOGTAG_MAIN, "Client disconnected");
                    closesocket(socket);
                    m_ClientRecvBuffers.erase(socket);
                    itr = m_ClientSockets.erase(itr);
                }
                else
                {
                    int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK)
                    {
                        // Not an actual error, there is just no data available right now
                        ++itr;
                    }
                    else
                    {
                        RB_LOG(LOGTAG_MAIN, "Client forcefully disconnected");
                        closesocket(socket);
                        m_ClientRecvBuffers.erase(socket);
                        itr = m_ClientSockets.erase(itr);
                    }
                }
            }
        }
        else
        {
            char temp[max_recv_chunk];
            int bytes_received = recv(m_HostConnection, temp, sizeof(temp), 0);

            if (bytes_received > 0)
            {
                AppendToRecvBuffer(m_HostRecvBuffer, temp, bytes_received);
                ExtractPackages(m_HostRecvBuffer, found_packages);
            }
            else if (bytes_received == 0)
            {
                RB_LOG(LOGTAG_MAIN, "Server disconnected");
                LeaveLobby();
            }
            else if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                RB_LOG(LOGTAG_MAIN, "Server forcefully disconnected");
                LeaveLobby();
            }
        }

        out_total_packages = static_cast<uint32_t>(found_packages.size());
        if (found_packages.empty())
            return nullptr;

        DataPackage* packages = m_FrameAllocator->Allocate<DataPackage>(found_packages.size());
        std::copy(found_packages.begin(), found_packages.end(), packages);
        return packages;
    }

    void WindowsNetworkService::ExtractPackages(RecvBuffer& buf, List<DataPackage>& out)
    {
        const size_t start_len = sizeof(START_MESSAGE);
        const size_t end_len   = sizeof(END_MESSAGE);

        size_t offset = 0;

        while (true)
        {
            int32_t start_rel = FindSequence(buf.data + offset, buf.length - offset, START_MESSAGE, start_len);
            if (start_rel < 0)
                break; // No message start found

            size_t start_idx     = offset + start_rel;
            size_t payload_start = start_idx + start_len;

            int32_t end_rel = FindSequence(buf.data + payload_start, buf.length - payload_start, END_MESSAGE, end_len);
            if (end_rel < 0)
            {
                // No message end found
                offset = start_idx;
                break;
            }

            size_t end_idx      = payload_start + end_rel;
            size_t payload_size = end_idx - payload_start;

            void* full_payload = m_FrameAllocator->Allocate(payload_size);
            memcpy(full_payload, buf.data + payload_start, payload_size);

            DataPackage pkg;
            pkg.data = full_payload;
            pkg.size = static_cast<uint32_t>(payload_size);
            out.push_back(pkg);

            // Keep scanning for more completed messages
            offset = end_idx + end_len;
        }

        if (offset > 0)
        {
            // Compact by shifting any unconsumed trailing bytes to the front of the buffer
            size_t remaining = buf.length - offset;
            if (remaining > 0)
                memmove(buf.data, buf.data + offset, remaining);
            buf.length = remaining;
        }
    }

    int32_t WindowsNetworkService::FindSequence(const char* haystack, size_t haystack_len, const char* needle, size_t needle_len)
    {
        if (needle_len == 0 || haystack_len < needle_len)
            return -1;

        for (size_t i = 0; i <= haystack_len - needle_len; ++i)
        {
            if (memcmp(haystack + i, needle, needle_len) == 0)
                return static_cast<int32_t>(i);
        }

        return -1;
    }

    void WindowsNetworkService::AppendToRecvBuffer(RecvBuffer& buf, const void* data, uint32_t size)
    {
        if (buf.length + size > sizeof(buf.data))
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Receive buffer overflow, dropping messages");
            buf.length = 0;
            return;
        }

        memcpy(buf.data + buf.length, data, size);
        buf.length += size;
    }

    void WindowsNetworkService::DisableBlocking(SOCKET& socket)
    {
        u_long mode = 1;
        ioctlsocket(socket, FIONBIO, &mode);
    }
}

#endif