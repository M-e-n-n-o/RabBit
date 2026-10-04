#include "RabBitCommon.h"
#include "PhysicsLayer.h"
#include "app/Application.h"
#include "entity/Scene.h"
#include "entity/components/PhysicsWorld.h"

#include <box2d/box2d.h>

using namespace RB::Entity;

namespace RB
{
    PhysicsLayer::PhysicsLayer()
        : ApplicationLayer("PhysicsLayer")
        , m_Accumulator(0.0f)
        , m_Alpha(0.0f)
    {
    }
    
    void PhysicsLayer::OnUpdate(float delta_time)
    {
        // TODO: Offload physics simulation to a different thread? (or add workers to b2WorldDef)

        m_Accumulator += Math::Min(delta_time, 0.25f);

        const auto& worlds2D = Application::GetInstance()->GetScene()->GetComponentsWithTypeOf<PhysicsWorld2D>();

        while (m_Accumulator >= PhysicsTickSpeedMs)
        {
            for (const PhysicsWorld2D* world : worlds2D)
            {
                world->PrePhysicsStep();
                b2World_Step(*world->GetWorldID(), PhysicsTickSpeedMs, world->GetSubStepCount());
            }
            m_Accumulator -= PhysicsTickSpeedMs;
        }

        m_Alpha = m_Accumulator / PhysicsTickSpeedMs;

        for (const PhysicsWorld2D* world : worlds2D)
        {
            world->PostPhysicsStep(m_Alpha);
        }
    }
}