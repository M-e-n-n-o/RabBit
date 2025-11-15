#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
    class DirectionalLight : public ObjectComponent
    {
    public:
        DirectionalLight(Math::Float3 direction, Math::Float3 color)
            : m_Direction(direction)
            , m_Color(color)
        {
        }

        Math::Float3    GetDirection() const { return m_Direction; }
        Math::Float3    GetColor() const { return m_Color; }

    private:
        Math::Float3    m_Direction;
        Math::Float3    m_Color;
    };
}