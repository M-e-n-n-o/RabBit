#include "RabBitCommon.h"
#include "NetworkTransformSync.h"

#include "entity/GameObject.h"

namespace RB::Entity
{
    NetworkTransformSync::NetworkTransformSync(bool is_local)
        : m_IsLocal(is_local)
    {
    }

    void NetworkTransformSync::OnAttached()
    {
        m_Transform = m_GameObject->GetComponent<Transform>();
    }

    void NetworkTransformSync::OnNetworkTick()
    {
        if (m_IsLocal)
        {
            Math::Float3 pos = m_Transform->GetPosition();
            SendMessage(&pos);
        }
    }

    void NetworkTransformSync::OnMessageReceived(uint64_t player_id, const Math::Float3* message)
    {
        if (!m_IsLocal)
        {
            m_Transform->SetPosition(*message);
        }
    }
}
