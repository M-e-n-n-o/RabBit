#pragma once
#include "PhysicsWorld.h"
#include "entity/ObjectComponent.h"
#include "entity/components/PhysicsShape.h"

#include <box2d/box2d.h>

namespace RB::Entity
{
    PhysicsWorld2D::PhysicsWorld2D(int sub_step_count, const Math::Float2& gravity)
        : m_SubStepCount(sub_step_count)
        , m_Gravity(gravity)
    {
        b2WorldDef world_def = b2DefaultWorldDef();
        world_def.gravity = { gravity.x, gravity.y };

        m_World = new b2WorldId();
        *m_World = b2CreateWorld(&world_def);
    }

    PhysicsWorld2D::~PhysicsWorld2D()
    {
        b2DestroyWorld(*m_World);
        delete m_World;
    }

    void PhysicsWorld2D::Register(PhysicsShape* shape)
    {
        m_Shapes.push_back(shape);
    }

    void PhysicsWorld2D::Unregister(PhysicsShape* shape)
    {
        std::erase(m_Shapes, shape);
    }

    void PhysicsWorld2D::PrePhysicsStep() const
    {
        for (PhysicsShape* shape : m_Shapes)
        {
            shape->PrePhysicsStep();
        }
    }

    void PhysicsWorld2D::PostPhysicsStep(float alpha) const
    {
        for (PhysicsShape* shape : m_Shapes)
        {
            shape->PostPhysicsStep(alpha);
        }
    }
}