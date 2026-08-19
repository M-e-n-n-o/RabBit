#ifdef RB_PLATFORM_WINDOWS
#include "RabBitCommon.h"
#include "WindowsNetworkService.h"

#include "app/Application.h"

#include <WS2tcpip.h>
#include <iphlpapi.h>

namespace RB
{
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

    uint64_t WindowsNetworkService::GetLobbyID() const
    {
        return 0;
    }

    void WindowsNetworkService::Broadcast(const DataPackage& package, bool reliable)
    {
        if (!m_IsHost || !m_HasNetworkConnection)
            return;

        for (auto itr = m_ClientSockets.begin(); itr != m_ClientSockets.end(); ++itr)
        {
            int result = send(*itr, (const char*)package.data, package.size, 0);

            if (result == SOCKET_ERROR)
            {
                RB_LOG_WARN(LOGTAG_MAIN, "Failed to broadcast data to a client");
            }
        }
    }

    void WindowsNetworkService::SendToHost(const DataPackage& package, bool reliable)
    {
        if (m_IsHost || !m_HasNetworkConnection)
            return;

        int result = send(m_HostConnection, (const char*)package.data, package.size, 0);

        if (result == SOCKET_ERROR)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Failed to send data to host");
        }
    }

    DataPackage* WindowsNetworkService::GetReceivedPackages(uint32_t& out_total_packages)
    {
        if (!m_HasNetworkConnection)
        {
            out_total_packages = 0;
            return nullptr;
        }

        const uint32_t max_size_per_package = 1024;

        if (m_IsHost)
        {
            if (m_ClientSockets.empty())
            {
                out_total_packages = 0;
                return nullptr;
            }

            DataPackage* packages = m_FrameAllocator->Allocate<DataPackage>(m_ClientSockets.size());
            uint32_t current_package = 0;

            for (auto itr = m_ClientSockets.begin(); itr != m_ClientSockets.end();)
            {
                SOCKET& socket = *itr;

                char* buffer = (char*)m_FrameAllocator->Allocate(max_size_per_package);

                int bytes_received = recv(socket, buffer, max_size_per_package, 0);

                if (bytes_received > 0)
                {
                    packages[current_package].data = buffer;
                    packages[current_package].size = bytes_received;
                    current_package++;

                    ++itr;
                }
                else if (bytes_received == 0)
                {
                    RB_LOG(LOGTAG_MAIN, "Client disconnected");

                    closesocket(socket);
                    itr = m_ClientSockets.erase(itr);
                }
                else if (bytes_received == SOCKET_ERROR)
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
                        itr = m_ClientSockets.erase(itr);
                    }
                }
            }

            out_total_packages = current_package;
            return packages;
        }
        else
        {
            DataPackage* package = m_FrameAllocator->Allocate<DataPackage>();

            char* buffer = (char*)m_FrameAllocator->Allocate(max_size_per_package);

            int bytes_received = recv(m_HostConnection, buffer, max_size_per_package, 0);

            if (bytes_received > 0)
            {
                package->data = buffer;
                package->size = bytes_received;

                out_total_packages = 1;
                return package;
            }
            else if (bytes_received == 0)
            {
                RB_LOG(LOGTAG_MAIN, "Server disconnected");
                LeaveLobby();
            }
            else if (bytes_received == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    RB_LOG(LOGTAG_MAIN, "Server forcefully disconnected");
                    LeaveLobby();
                }
            }

            out_total_packages = 0;
            return nullptr;
        }
    }

    void WindowsNetworkService::DisableBlocking(SOCKET& socket)
    {
        u_long mode = 1;
        ioctlsocket(socket, FIONBIO, &mode);
    }
}

#endif