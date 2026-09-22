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
            , scale(1.0f)
            , rotation()
        {}

        // Local transform variables
        Math::Float3     position;
        Math::Float3     scale;
        Math::Quaternion rotation;

        // Transformation matrix
        Math::Float4x4 GetLocalToWorldMatrix() const;
    };

    static const Transform* GetDefaultTransform()
    {
        static Transform t = Transform();
        return &t;
    }
}