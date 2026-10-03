#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
    class Transform : public ObjectComponent
    {
    public:
        Transform()
            : m_Position(0.0f)
            , m_Scale(1.0f)
            , m_Rotation()
            , m_CachedDirty(true)
        {}

        // -------------------------------------------------------
        // Getters
        const Math::Float3&     GetPosition() const { return m_Position; }
        const Math::Quaternion& GetRotation() const { return m_Rotation; }
        const Math::Float3&     GetScale()    const { return m_Scale; }

        float GetPositionX() const { return m_Position.x; }
        float GetPositionY() const { return m_Position.y; }
        float GetPositionZ() const { return m_Position.z; }

        float GetScaleX() const { return m_Scale.x; }
        float GetScaleY() const { return m_Scale.y; }
        float GetScaleZ() const { return m_Scale.z; }

        // Transformation matrix
        const Math::Float4x4& GetLocalToWorldMatrix() const;

        // -------------------------------------------------------
        // Setters
        void SetPosition(const Math::Float3& pos);
        void SetRotation(const Math::Quaternion& rot);
        void SetScale(const Math::Float3& scale);

        void SetPositionX(float x);
        void SetPositionY(float y);
        void SetPositionZ(float z);

        void SetScaleX(float x);
        void SetScaleY(float y);
        void SetScaleZ(float z);

        void NormalizeRotation();

    private:
        void MarkDirty();

        void OnNewParent(GameObject* obj) override;

        // Local transform variables
        Math::Float3             m_Position;
        Math::Float3             m_Scale;
        Math::Quaternion         m_Rotation;

        // Simple caching to make the GetLocalToWorldMatrix method cheaper
        mutable Math::Float4x4   m_CachedLocalToWorld;
        mutable bool             m_CachedDirty;
    };
}