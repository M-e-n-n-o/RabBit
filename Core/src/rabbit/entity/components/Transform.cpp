#include "RabBitCommon.h"
#include "Transform.h"
#include "entity/GameObject.h"

namespace RB::Entity
{
    Math::Float4x4 Transform::GetLocalToWorldMatrix() const
    {
        Math::Float4x4 m = GetWorldRotation().ToMatrix();
        m.Scale(GetWorldScale());
        m.SetPosition(GetWorldPosition());
        return m;
    }

    Math::Float3 Transform::GetWorldPosition() const
    {
        Math::Float3 world_pos = position;

        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const Transform* parent_transform = parent_obj->GetComponent<Transform>();
            world_pos = world_pos + parent_transform->position;
            parent_obj = parent_obj->GetParent();
        }

        return world_pos;
    }

    Math::Quaternion Transform::GetWorldRotation() const
    {
        Math::Quaternion world_rot = rotation;

        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const Transform* parent_transform = parent_obj->GetComponent<Transform>();
            world_rot = world_rot * parent_transform->rotation;
            parent_obj = parent_obj->GetParent();
        }

        world_rot.Normalize();
        return world_rot;
    }

    Math::Float3 Transform::GetWorldScale() const
    {
        Math::Float3 world_scale = scale;

        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const Transform* parent_transform = parent_obj->GetComponent<Transform>();
            world_scale = world_scale * parent_transform->scale;
            parent_obj = parent_obj->GetParent();
        }

        return world_scale;
    }
}