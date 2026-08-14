#ifdef RB_PLATFORM_WINDOWS
#include "RabBitCommon.h"
#include "WindowsNetworkService.h"

#include <WS2tcpip.h>
#include <iphlpapi.h>

namespace RB
{
    WindowsNetworkService::WindowsNetworkService()
        : m_HasNetworkConnection(false)
        , m_IsHost(false)
        , m_TcpListenSocket(INVALID_SOCKET)
        , m_HostConnection(INVALID_SOCKET)
    {
        WSAData wsa_data;

        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Windows network startup failed with code: %d", result);
            m_IsValid = false;
            return;
        }

        m_IsValid = true;
    }

    WindowsNetworkService::~WindowsNetworkService()
    {
        if (!m_IsValid)
            return;

        LeaveLobby();
        WSACleanup();
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

        // Disable blocking mode on the socket (so the accept() call does not block)
        u_long mode = 1;
        ioctlsocket(m_TcpListenSocket, FIONBIO, &mode);

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
    }

    void WindowsNetworkService::SendToHost(const DataPackage& package, bool reliable)
    {
    }

    DataPackage* WindowsNetworkService::GetReceivedPackages(uint32_t& out_total_packages)
    {
        return nullptr;
    }
}

#endif