#pragma once

#ifdef RB_PLATFORM_WINDOWS

#include "app/PlatformService.h"

#include "platform/utils/Windows.h"
#include <WinSock2.h>

namespace RB
{
    // Only intended to be used for debugging purposes!

    struct FrameAllocator;

    class WindowsNetworkService : public PlatformNetworkService
    {
    public:
        WindowsNetworkService();
        ~WindowsNetworkService();

        bool IsInitialized() const override { return m_IsValid; }

        void CreateLobby(LobbyType lobby_type, uint32_t max_members) override;

        // Pass in the IP address to connect to as char*
        void JoinLobby(uint64_t lobby_id) override;
        void LeaveLobby() override;

        bool IsConnected() const override;

        uint64_t GetLobbyID() const override;

        void Broadcast(const DataPackage& package, bool reliable) override;
        void SendToHost(const DataPackage& package, bool reliable) override;

        DataPackage* GetReceivedPackages(uint32_t& out_total_packages) override;

        // Not a thing
        void OpenInviteFriendsOverlay() override {}

    private:
        static const uint32_t c_Port = 42069;

        bool            m_IsValid;
        bool            m_HasNetworkConnection;
        bool            m_IsHost;
        SOCKET          m_TcpListenSocket;
        List<SOCKET>    m_ClientSockets;
        SOCKET          m_HostConnection;

        FrameAllocator* m_FrameAllocator;
    };
}
#endif