#pragma once

#include "app/NetworkRouter.h"
#include "entity/ObjectComponent.h"

#include "Transform.h"

namespace RB::Entity
{
    class NetworkTransform : public ObjectComponent, NetworkHandler<Transform>
    {
    public:
        NetworkTransform(bool is_local);

        void OnAttached() override;

        void Update() override;

        void OnMessageReceived(uint64_t player_id, const Transform* message) override;

    private:
        bool m_IsLocal;
        Transform* m_Transform;
    };
}