#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
    class Transform : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("Transform");

        Transform()
            : position(0.0f)
            , rotation(0.0f)
            , scale(1.0f)
        {}

        Math::Float3 position;
        Math::Float3 rotation;
        Math::Float3 scale;

        // Transformation matrix
        Math::Float4x4 GetLocalToWorldMatrix() const
        {
            Math::Float4x4 m;
            m.RotateAroundX(Math::DegreesToRadians(rotation.x));
            m.RotateAroundY(Math::DegreesToRadians(rotation.y));
            m.RotateAroundZ(Math::DegreesToRadians(rotation.z));
            m.SetPosition(position);
            m.Scale(scale);

            return m;
        }
    };
}