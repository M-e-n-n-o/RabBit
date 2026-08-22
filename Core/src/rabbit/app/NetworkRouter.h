#pragma once

#include "RabBitCommon.h"
#include "ApplicationLayer.h"

namespace RB
{
    class PlatformNetworkService;

    namespace NetworkUtils
    {
        template <typename T>
        constexpr std::string_view RawTypeName()
        {
#if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
            return __FUNCSIG__;
#endif
        }

        template <typename T>
        constexpr uint64_t TypeId()
        {
            uint64_t h = 0;
            std::string_view s = RawTypeName<T>();
            for (uint8_t c : s)
                HashCombine(h, c);
            return h;
        }

        class NetworkHandlerBase
        {
        public:
            virtual ~NetworkHandlerBase() = default;
            virtual bool ListensToID(uint64_t id) const = 0;
            virtual void OnNetworkMessage(uint64_t player_id, uint64_t message_id, const uint8_t* data, uint64_t size) = 0;
        };

        template <typename T>
        class NetworkHandlerSingle
        {
        public:
            virtual ~NetworkHandlerSingle() = default;
            virtual void OnMessageReceived(uint64_t player_id, const T* message) = 0;
        };
    }

    class NetworkRouterLayer : public ApplicationLayer
    {
    public:
        NetworkRouterLayer(PlatformNetworkService* network_service);
        ~NetworkRouterLayer();

        void OnUpdate(float delta_time) override;

        void SendMessage(uint64_t message_id, uint8_t* data, uint64_t size, bool reliable);

        void AddHandler(NetworkUtils::NetworkHandlerBase* handler);
        void RemoveHandler(NetworkUtils::NetworkHandlerBase* handler);

        static NetworkRouterLayer* GetInstance() { return s_Instance; }

    private:
        PlatformNetworkService*                 m_NetworkService;
        List<NetworkUtils::NetworkHandlerBase*> m_Handlers;

        static NetworkRouterLayer*              s_Instance;
    };

    template <typename... T>
    class NetworkHandler : public NetworkUtils::NetworkHandlerBase, public NetworkUtils::NetworkHandlerSingle<T>...
    {
    public:
        NetworkHandler()
        {
            m_Router = NetworkRouterLayer::GetInstance();
            RB_ASSERT_FATAL(LOGTAG_MAIN, m_Router, "NetworkRouterLayer is null, is AppInfo::useNetworking set to true?");
            m_Router->AddHandler(this);
        }
        ~NetworkHandler()
        { 
            m_Router->RemoveHandler(this);
        }

        template <typename M>
        void SendMessage(M* message, bool reliable = false)
        {
            RB_STATIC_ASSERT((std::is_same_v<M, T> || ...), "Message type must be one of NetworkNetworkHandler's types");
            m_Router->SendMessage(NetworkUtils::TypeId<M>(), (uint8_t*)message, sizeof(M), reliable);
        }

        bool ListensToID(uint64_t id) const override
        {
            return ((NetworkUtils::TypeId<T>() == id) || ...);
        }

        void OnNetworkMessage(uint64_t player_id, uint64_t message_id, const uint8_t* data, uint64_t size) override
        {
            (DispatchOne<T>(player_id, message_id, data, size), ...);
        }

    private:
        template <typename M>
        void DispatchOne(uint64_t player_id, uint64_t message_id, const uint8_t* data, uint64_t size)
        {
            if (message_id == NetworkUtils::TypeId<M>())
            {
                if (size == sizeof(M))
                    static_cast<NetworkUtils::NetworkHandlerSingle<M>*>(this)->OnMessageReceived(player_id, (M*)data);
                else
                    RB_LOG_ERROR(LOGTAG_MAIN, "Identified network message is not the expected size");
            }
        }

        NetworkRouterLayer* m_Router;
    };
}