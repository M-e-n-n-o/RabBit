#include "RabBitCommon.h"
#include "NetworkTransform.h"

#include "entity/GameObject.h"

namespace RB::Entity
{
    NetworkTransform::NetworkTransform(bool is_local)
        : m_IsLocal(is_local)
    {
    }

    void NetworkTransform::OnAttached()
    {
        m_Transform = m_GameObject->GetComponent<Transform>();
    }

    void NetworkTransform::Update()
    {
        if (m_IsLocal)
        {
            SendMessage(m_Transform);
        }
    }

    void NetworkTransform::OnMessageReceived(uint64_t player_id, const Transform* message)
    {
        if (!m_IsLocal)
        {
            m_Transform->position = message->position;
        }
    }
}
