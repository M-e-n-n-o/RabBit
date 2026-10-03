#include "RabBitCommon.h"
#include "Transform.h"
#include "entity/GameObject.h"

namespace RB::Entity
{
    const Math::Float4x4& Transform::GetLocalToWorldMatrix() const
    {
        if (!m_CachedDirty)
        {
            return m_CachedLocalToWorld;
        }

        m_CachedLocalToWorld = m_Rotation.ToMatrix();
        m_CachedLocalToWorld.Scale(m_Scale);
        m_CachedLocalToWorld.SetPosition(m_Position);

        GameObject* parent_obj = m_GameObject->GetParent();
        if (parent_obj)
        {
            const Transform* parent_transform = parent_obj->GetComponent<Transform>();
            if (parent_transform != nullptr)
                m_CachedLocalToWorld = m_CachedLocalToWorld * parent_transform->GetLocalToWorldMatrix();
        }

        m_CachedDirty = false;
        return m_CachedLocalToWorld;
    }

    void Transform::SetPosition(const Math::Float3& pos)
    {
        m_Position = pos;
        MarkDirty();
    }

    void Transform::SetRotation(const Math::Quaternion& rot)
    {
        m_Rotation = rot;
        MarkDirty();
    }

    void Transform::SetScale(const Math::Float3& scale)
    {
        m_Scale = scale;
        MarkDirty();
    }

    void Transform::SetPositionX(float x)
    {
        m_Position.x = x;
        MarkDirty();
    }

    void Transform::SetPositionY(float y)
    {
        m_Position.y = y;
        MarkDirty();
    }

    void Transform::SetPositionZ(float z)
    {
        m_Position.z = z;
        MarkDirty();
    }

    void Transform::SetScaleX(float x)
    {
        m_Scale.x = x;
        MarkDirty();
    }

    void Transform::SetScaleY(float y)
    {
        m_Scale.y = y;
        MarkDirty();
    }

    void Transform::SetScaleZ(float z)
    {
        m_Scale.z = z;
        MarkDirty();
    }

    void Transform::NormalizeRotation()
    {
        m_Rotation.Normalize();
        MarkDirty();
    }

    void Transform::MarkDirty()
    {
        if (m_CachedDirty)
            return;

        m_CachedDirty = true;

        for (GameObject* child : m_GameObject->GetChildren())
        {
            if (Transform* t = child->GetComponent<Transform>())
                t->MarkDirty();
        }
    }

    void Transform::OnNewParent(GameObject* obj)
    {
        MarkDirty();
    }
}