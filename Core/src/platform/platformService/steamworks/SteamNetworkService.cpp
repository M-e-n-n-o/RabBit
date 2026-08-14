#ifdef RB_STEAM_API
#include "RabBitCommon.h"
#include "SteamNetworkService.h"

#include "app/Application.h"

namespace RB
{
    SteamNetworkService::SteamNetworkService()
        : m_RequestingConnection(false)
        , m_HasNetworkConnection(false)
        , m_IsHost(false)
        , m_LobbyID(k_steamIDNil)
        , m_ListenSocket(k_HSteamListenSocket_Invalid)
        , m_HostConnection(k_HSteamNetConnection_Invalid)
        , m_PollGroup(k_HSteamNetPollGroup_Invalid)
        , m_FrameAllocator(nullptr)
    {
        if (!SteamAPI_IsSteamRunning())
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Steam is not running, cannot create the SteamNetworkService");
            m_IsValid = false;
            return;
        }

        if (!SteamUser()->BLoggedOn())
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "User is not logged in, cannot create the SteamNetworkService");
            m_IsValid = false;
            return;
        }

        m_FrameAllocator = Application::GetInstance()->GetAllocator();
        SteamNetworkingUtils()->InitRelayNetworkAccess();

        m_IsValid = true;
    }

    SteamNetworkService::~SteamNetworkService()
    {
        if (!m_IsValid)
            return;

        LeaveLobby();
    }

    void SteamNetworkService::CreateLobby(LobbyType lobby_type, uint32_t max_members)
    {
        if (m_RequestingConnection)
        {
            // Already called this method
            return;
        }

        if (m_HasNetworkConnection)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Make sure to leave the current lobby before creating a new one");
            return;
        }

        m_RequestingConnection = true;
        m_IsHost = true;

        ELobbyType type = k_ELobbyTypePrivate;
        switch (lobby_type)
        {
        case RB::LobbyType::Private:     type = k_ELobbyTypePrivate; break;
        case RB::LobbyType::FriendsOnly: type = k_ELobbyTypeFriendsOnly; break;
        case RB::LobbyType::Public:      type = k_ELobbyTypePublic; break;
        }

        RB_LOG(LOGTAG_MAIN, "Schedule create lobby");
        SteamMatchmaking()->CreateLobby(type, max_members);
    }

    void SteamNetworkService::JoinLobby(uint64_t lobby_id)
    {
        if (m_RequestingConnection)
        {
            // Already called this method
            return;
        }

        if (m_HasNetworkConnection)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Make sure to leave the current lobby before joining a new one");
            return;
        }

        m_RequestingConnection = true;
        m_IsHost = false;

        CSteamID id = CSteamID();
        id.SetFromUint64(lobby_id);

        RB_LOG(LOGTAG_MAIN, "Requesting to join lobby with ID: %d", id);
        SteamMatchmaking()->JoinLobby(id);
    }

    void SteamNetworkService::LeaveLobby()
    {
        if (!m_LobbyID.IsValid())
            return;

        RB_LOG(LOGTAG_MAIN, "Leaving lobby");

        if (m_IsHost)
        {
            for (HSteamNetConnection conn : m_ClientConnections)
                SteamNetworkingSockets()->CloseConnection(conn, 0, nullptr, false);
            m_ClientConnections.clear();

            if (m_ListenSocket != k_HSteamListenSocket_Invalid)
                SteamNetworkingSockets()->CloseListenSocket(m_ListenSocket);
        }
        else
        {
            SteamNetworkingSockets()->CloseConnection(m_HostConnection, 0, nullptr, false);
        }

        SteamNetworkingSockets()->DestroyPollGroup(m_PollGroup);

        SteamMatchmaking()->LeaveLobby(m_LobbyID);
        
        m_IsHost = false;
        m_HasNetworkConnection = false;
        m_LobbyID = k_steamIDNil;
        m_ListenSocket = k_HSteamListenSocket_Invalid;
        m_HostConnection = k_HSteamNetConnection_Invalid;
        m_PollGroup = k_HSteamNetPollGroup_Invalid;
    }

    bool SteamNetworkService::IsConnected() const
    {
        return m_HasNetworkConnection;
    }

    uint64_t SteamNetworkService::GetLobbyID() const
    {
        RB_ASSERT(LOGTAG_MAIN, m_HasNetworkConnection, "Lobby ID is invalid as you are currently not connected to a lobby");
        return m_LobbyID.ConvertToUint64();
    }

    void SteamNetworkService::Broadcast(const DataPackage& package, bool reliable)
    {
        if (!m_IsHost || !m_HasNetworkConnection)
            return;

        for (const auto& conn : m_ClientConnections)
            SendToConnection(conn, package, reliable);
    }

    void SteamNetworkService::SendToHost(const DataPackage& package, bool reliable)
    {
        if (m_IsHost || !m_HasNetworkConnection)
            return;

        SendToConnection(m_HostConnection, package, reliable);
    }

    DataPackage* SteamNetworkService::GetReceivedPackages(uint32_t& out_total_packages)
    {
        if (!m_HasNetworkConnection)
        {
            out_total_packages = 0;
            return nullptr;
        }

        const uint32_t max_packages_per_client = 32;
        const uint32_t max_packages = m_IsHost ? m_ClientConnections.size() * max_packages_per_client : max_packages_per_client;

        if (max_packages <= 0)
        {
            // No clients connected
            out_total_packages = 0;
            return nullptr;
        }

        SteamNetworkingMessage_t** msgs = (SteamNetworkingMessage_t**)m_FrameAllocator->Allocate(sizeof(SteamNetworkingMessage_t*) * max_packages);
        SteamDataPackage* packages = m_FrameAllocator->Allocate<SteamDataPackage>(max_packages);

        int total_messages = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup(m_PollGroup, msgs, max_packages);

        if (total_messages < 0)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Failed to receive messages");
            out_total_packages = 0;
            return nullptr;
        }

        for (int i = 0; i < total_messages; i++)
        {
            packages[i].data = msgs[i]->GetData();
            packages[i].size = msgs[i]->GetSize();
            packages[i].steamMsg = msgs[i];
        }

        out_total_packages = total_messages;

        return packages;
    }

    void SteamNetworkService::SendToConnection(HSteamNetConnection conn, const DataPackage& package, bool reliable)
    {
        SteamNetworkingSockets()->SendMessageToConnection(conn, package.data, package.size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
    }

    void SteamNetworkService::OpenInviteFriendsOverlay()
    {
        if (!m_IsHost || !m_HasNetworkConnection)
            return;

        SteamFriends()->ActivateGameOverlayInviteDialog(m_LobbyID);
    }

    // ---------------------------------------------------------------------------
    //								Callback functions
    // ---------------------------------------------------------------------------

    void SteamNetworkService::OnLobbyEntered(LobbyEnter_t* info)
    {
        // This is also called when creating a lobby (after OnLobbyCreated)
        if (m_IsHost)
            return;

        m_HasNetworkConnection = true;
        m_RequestingConnection = false;

        m_LobbyID = CSteamID(info->m_ulSteamIDLobby);
        RB_LOG(LOGTAG_MAIN, "Joined lobby with ID: %d", m_LobbyID);

        const char* host_str = SteamMatchmaking()->GetLobbyData(m_LobbyID, "host_steamid");
        CSteamID host_id(std::strtoull(host_str, nullptr, 10));

        SteamNetworkingIdentity identity;
        identity.SetSteamID(host_id);

        SteamNetworkingConfigValue_t opt = {};
        m_HostConnection = SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, &opt);

        m_PollGroup = SteamNetworkingSockets()->CreatePollGroup();
        SteamNetworkingSockets()->SetConnectionPollGroup(m_HostConnection, m_PollGroup);
    }

    void SteamNetworkService::OnLobbyCreated(LobbyCreated_t* info)
    {
        if (info->m_eResult != k_EResultOK)
            return;

        m_HasNetworkConnection = true;
        m_RequestingConnection = false;

        m_LobbyID = CSteamID(info->m_ulSteamIDLobby);
        RB_LOG(LOGTAG_MAIN, "Created lobby with ID: %d", m_LobbyID);

        // Store host's SteamID in lobby data so joiners can read it directly.
        uint64_t user_id = SteamUser()->GetSteamID().ConvertToUint64();
        SteamMatchmaking()->SetLobbyData(m_LobbyID, "host_steamid", std::to_string(user_id).c_str());

        SteamNetworkingConfigValue_t opt = {};
        m_ListenSocket = SteamNetworkingSockets()->CreateListenSocketP2P(0, 0, &opt);

        m_PollGroup = SteamNetworkingSockets()->CreatePollGroup();
    }

    void SteamNetworkService::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
    {
        switch (info->m_info.m_eState)
        {
        case k_ESteamNetworkingConnectionState_Connecting:
        {
            if (m_IsHost)
            {
                RB_ASSERT(LOGTAG_MAIN, SteamNetworkingSockets()->AcceptConnection(info->m_hConn) == k_EResultOK, "Failed to accept steam connection");
                SteamNetworkingSockets()->SetConnectionPollGroup(info->m_hConn, m_PollGroup);
            }
        }
        break;

        case k_ESteamNetworkingConnectionState_Connected:
        {
            if (m_IsHost)
            {
                RB_LOG(LOGTAG_MAIN, "Client connected");
                m_ClientConnections.push_back(info->m_hConn);
            }
            else
            {
                RB_LOG(LOGTAG_MAIN, "Connected to host");
            }
        }
        break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            if (m_IsHost)
            {
                RB_LOG(LOGTAG_MAIN, "Client disconnected");
                SteamNetworkingSockets()->CloseConnection(info->m_hConn, 0, nullptr, false);
                m_ClientConnections.erase(std::remove(m_ClientConnections.begin(), m_ClientConnections.end(), info->m_hConn), m_ClientConnections.end());
            }
            else if (info->m_hConn == m_HostConnection)
            {
                RB_LOG(LOGTAG_MAIN, "Disconnected from host");
                LeaveLobby();
            }
        }
        break;

        default:
            break;
        }
    }

    void SteamNetworkService::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* info)
    {
        JoinLobby(info->m_steamIDLobby.ConvertToUint64());
    }
}
#endif