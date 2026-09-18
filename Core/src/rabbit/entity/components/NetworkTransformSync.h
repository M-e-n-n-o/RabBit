#pragma once

#include "app/NetworkRouter.h"
#include "entity/ObjectComponent.h"

#include "Transform.h"

namespace RB::Entity
{
    class NetworkTransformSync : public ObjectComponent, public NetworkHandler<Math::Float3>
    {
    public:
        NetworkTransformSync(bool is_local);

        void OnAttached() override;

        void OnNetworkTick() override;

        void OnMessageReceived(uint64_t player_id, const Math::Float3* message) override;

    private:
        bool m_IsLocal;
        Transform* m_Transform;
    };
}