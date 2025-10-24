#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
    class Transform : public ObjectComponent
    {
    public:
        Transform()
            : position(0.0f)
            , rotation(0.0f)
            , scale(1.0f)
        {}

        // Local transform variables
        Math::Float3 position;
        Math::Float3 rotation;
        Math::Float3 scale;

        // Real transforms, based on Parent transforms
        Math::Float3 GetWorldPosition() const;
        Math::Float3 GetWorldRotation() const;
        Math::Float3 GetWorldScale() const;

        // Transformation matrix
        Math::Float4x4 GetLocalToWorldMatrix() const;
    };

    static const Transform* GetDefaultTransform()
    {
        static Transform t = Transform();
        return &t;
    }
}