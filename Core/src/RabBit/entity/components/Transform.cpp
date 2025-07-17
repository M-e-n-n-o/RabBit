#include "RabBitCommon.h"
#include "Transform.h"
#include "entity/GameObject.h"

namespace RB::Entity
{
    Math::Float4x4 Transform::GetLocalToWorldMatrix() const
    {
        const Math::Float3& world_rot = GetWorldRotation();

        static_assert(false);
        // TODO: Bij de rotation moet een axis worden toegevoegd, zie Math.cpp (CreateModelMatrix) in ByteCat
        // DOE DIT OOK BIJ HET MAKEN VAN DE VIEW MATRIX!!!! (Frustum::SetTransform)

        Math::Float4x4 m;
        m.RotateAroundX(Math::DegreesToRadians(world_rot.x));
        m.RotateAroundY(Math::DegreesToRadians(world_rot.y));
        m.RotateAroundZ(Math::DegreesToRadians(world_rot.z));
        m.SetPosition(GetWorldPosition());
        m.Scale(GetWorldScale());

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

    Math::Float3 Transform::GetWorldRotation() const
    {
        Math::Float3 world_rot = rotation;

        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const Transform* parent_transform = parent_obj->GetComponent<Transform>();
            world_rot = world_rot + parent_transform->rotation;
            parent_obj = parent_obj->GetParent();
        }

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