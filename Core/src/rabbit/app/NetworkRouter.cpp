#include "RabBitCommon.h"
#include "NetworkRouter.h"
#include "PlatformService.h"
#include "Application.h"

namespace RB
{
    NetworkRouterLayer* NetworkRouterLayer::s_Instance = nullptr;

    struct DataHeader
    {
        uint64_t playerID;
        uint64_t messageID;
        uint8_t  isReliable;
        uint8_t  padding[7];
    };

    NetworkRouterLayer::NetworkRouterLayer(PlatformNetworkService* network_service)
        : ApplicationLayer("NetworkRouterLayer")
        , m_NetworkService(network_service)
    {
        RB_ASSERT_FATAL(LOGTAG_MAIN, s_Instance == nullptr, "NetworkRouterLayer already exists");
        s_Instance = this;
    }

    NetworkRouterLayer::~NetworkRouterLayer()
    {
        s_Instance = nullptr;
    }

    void NetworkRouterLayer::OnUpdate(float delta_time)
    {
        if (!m_NetworkService)
            return;

        m_NetworkService->Update();

        if (!m_NetworkService->IsConnected())
            return;

        uint32_t count;
        DataPackage* packages = m_NetworkService->GetReceivedPackages(count);

        if (count == 0)
            return;

        const uint32_t header_size = sizeof(DataHeader);

        for (int i = 0; i < count; i++)
        {
            const DataPackage* package = &packages[i];

            if (package->size <= header_size)
            {
                RB_LOG_WARN(LOGTAG_MAIN, "Received network message is invalid");
                continue;
            }

            const uint8_t* data_ptr = (const uint8_t*)package->data;
            uint64_t remaining_size = package->size;

            const DataHeader* header = (const DataHeader*)data_ptr;
            data_ptr += header_size;
            remaining_size -= header_size;

            if (header->playerID == m_NetworkService->GetPlayerID())
                continue;

            if (m_NetworkService->IsHost())
            {
                // Forward message to all clients
                m_NetworkService->Broadcast(package, header->isReliable);
            }

            for (const auto& handler : m_Handlers)
            {
                if (handler->ListensToID(header->messageID))
                    handler->OnNetworkMessage(header->playerID, header->messageID, data_ptr, remaining_size);
            }
        }
    }

    void NetworkRouterLayer::SendMessage(uint64_t message_id, uint8_t* data, uint64_t size, bool reliable)
    {
        if (!m_NetworkService || !m_NetworkService->IsConnected())
            return;

        DataPackage package = {};
        package.size = size + sizeof(DataHeader);

        if (package.size > 1024)
            package.data = Application::GetInstance()->GetAllocator()->Allocate(package.size);
        else
            package.data = ALLOC_STACK(package.size);

        uint8_t* data_ptr = (uint8_t*)package.data;
        DataHeader* header = (DataHeader*)data_ptr;
        data_ptr += sizeof(DataHeader);

        header->playerID   = m_NetworkService->GetPlayerID();
        header->messageID  = message_id;
        header->isReliable = (uint8_t)reliable;

        memcpy(data_ptr, data, size);

        if (m_NetworkService->IsHost())
            m_NetworkService->Broadcast(&package, reliable);
        else
            m_NetworkService->SendToHost(&package, reliable);
    }

    void NetworkRouterLayer::AddHandler(NetworkUtils::NetworkHandlerBase* handler)
    {
        m_Handlers.push_back(handler);
    }

    void NetworkRouterLayer::RemoveHandler(NetworkUtils::NetworkHandlerBase* handler)
    {
        std::erase(m_Handlers, handler);
    }
}