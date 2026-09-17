#pragma once

#include "app/NetworkRouter.h"
#include "entity/ObjectComponent.h"

#include "Transform.h"

namespace RB::Entity
{
    class NetworkTransformSync : public ObjectComponent, NetworkHandler<Math::Float3>
    {
    public:
        NetworkTransformSync(bool is_local);

        void OnAttached() override;

        void Update() override;

        void OnMessageReceived(uint64_t player_id, const Math::Float3* message) override;

    private:
        bool m_IsLocal;
        Transform* m_Transform;
    };
}