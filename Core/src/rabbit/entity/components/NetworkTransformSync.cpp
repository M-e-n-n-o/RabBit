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

    void NetworkTransformSync::Update()
    {
        if (m_IsLocal)
        {
            SendMessage(m_Transform);
        }
    }

    void NetworkTransformSync::OnMessageReceived(uint64_t player_id, const Transform* message)
    {
        if (!m_IsLocal)
        {
            m_Transform->position = message->position;
        }
    }
}
