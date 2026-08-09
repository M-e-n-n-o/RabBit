#pragma once

namespace RB
{
    enum class PlatformAPI
    {
        None,
        Steamworks
    };

    enum class LobbyType
    {
        Private,
        FriendsOnly,
        Public
    };

    // !!! Most of the methods in these classes are async and will not be completed directly after calling them !!!

    class PlatformService
    {
    public:
        virtual ~PlatformService() = default;

        virtual bool IsInitialized() const = 0;

        virtual void Update() = 0;

        static PlatformAPI GetAPI() { return s_Api; }
        static PlatformService* Create(PlatformAPI api);

    protected:
        PlatformService() = default;

    private:
        inline static PlatformAPI s_Api = PlatformAPI::None;
    };

    struct DataPackage
    {
        const void* data;
        uint32_t size;

        DataPackage() : data(nullptr), size(0) {}
        virtual ~DataPackage() = default;
    };

    // This class should be created on App side if needed
    class PlatformNetworkService
    {
    public:
        virtual ~PlatformNetworkService() = default;

        virtual bool IsInitialized() const = 0;

        virtual void CreateLobby(LobbyType lobby_type, uint32_t max_members) = 0;

        virtual void JoinLobby(uint64_t lobby_id) = 0;
        virtual void LeaveLobby() = 0;

        virtual bool IsConnected() const = 0;
        virtual uint64_t GetLobbyID() const = 0;

        virtual void Broadcast(const DataPackage& package, bool reliable) = 0;  // Host only
        virtual void SendToHost(const DataPackage& package, bool reliable) = 0; // Client only

        // Returned messages are only valid during the lifetime of the current frame on the main thread (allocated by the FrameAllocator from Application)
        virtual DataPackage* GetReceivedPackages(uint32_t& out_total_packages) = 0;

        virtual void OpenInviteFriendsOverlay() = 0; // Host only

        // Create this AFTER the platform service API has been set!
        static PlatformNetworkService* Create();

    protected:
        PlatformNetworkService() = default;
    };
}