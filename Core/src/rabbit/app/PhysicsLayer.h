#pragma once

#include "app/ApplicationLayer.h"

namespace RB
{
    static const float PhysicsTickSpeedMs = 1.0f / 60.0f;

    class PhysicsLayer : public ApplicationLayer
    {
    public:
        PhysicsLayer();

        void OnUpdate(float delta_time) override;

        float GetInterpolationAlpha() const { return m_Alpha; }

    private:
        float m_Accumulator;
        float m_Alpha;
    };
}