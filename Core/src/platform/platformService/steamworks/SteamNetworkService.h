#pragma once
#ifdef RB_STEAM_API

#include "app/PlatformService.h"

#include <steam/steam_api.h>

namespace RB
{
    struct FrameAllocator;

    struct SteamDataPackage : public DataPackage
    {
        SteamNetworkingMessage_t* steamMsg;

        SteamDataPackage()
            : DataPackage()
            , steamMsg(nullptr)
        {
        }

        ~SteamDataPackage()
        {
            SAFE_RELEASE(steamMsg);
        }
    };

    class SteamNetworkService : public PlatformNetworkService
    {
    public:
        SteamNetworkService();
        ~SteamNetworkService();

        bool IsInitialized() const override { return m_IsValid; }

        void Update() override;

        void CreateLobby(LobbyType lobby_type, uint32_t max_members) override;

        void JoinLobby(uint64_t lobby_id) override;
        void LeaveLobby() override;

        bool IsConnected() const override;
        bool IsHost() const override;

        uint64_t GetLobbyID() const override;
        uint64_t GetPlayerID() const override;

        void Broadcast(const DataPackage* package, bool reliable) override;
        void SendToHost(const DataPackage* package, bool reliable) override;

        DataPackage* GetReceivedPackages(uint32_t& out_total_packages) override;

        void OpenInviteFriendsOverlay() override;

    private:
        void SendToConnection(HSteamNetConnection conn, const DataPackage* package, bool reliable);

        STEAM_CALLBACK(SteamNetworkService, OnLobbyEntered, LobbyEnter_t);
        STEAM_CALLBACK(SteamNetworkService, OnLobbyCreated, LobbyCreated_t);
        STEAM_CALLBACK(SteamNetworkService, OnConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);
        STEAM_CALLBACK(SteamNetworkService, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t);

        bool                        m_IsValid;
        bool                        m_RequestingConnection;
        bool                        m_HasNetworkConnection;
        bool                        m_IsHost;
        uint64_t                    m_PlayerID;
        CSteamID                    m_LobbyID;
        HSteamListenSocket          m_ListenSocket;
        List<HSteamNetConnection>   m_ClientConnections;
        HSteamNetConnection         m_HostConnection;
        HSteamNetPollGroup          m_PollGroup;

        FrameAllocator*             m_FrameAllocator;
    };
}
#endif