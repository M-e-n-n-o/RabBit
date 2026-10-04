#pragma once
#include "entity/ObjectComponent.h"
#include "math/Vector.h"

struct b2WorldId;

namespace RB::Entity
{
    class PhysicsShape;

    class PhysicsWorld2D : public ObjectComponent
    {
    public:
        PhysicsWorld2D(int sub_step_count = 4, const Math::Float2& gravity = Math::Float2(0.0f, -9.8f));
        ~PhysicsWorld2D();

        void Register(PhysicsShape* shape);
        void Unregister(PhysicsShape* shape);

        void PrePhysicsStep() const;
        void PostPhysicsStep(float alpha) const;

        const b2WorldId* GetWorldID() const { return m_World; }
        const int GetSubStepCount() const { return m_SubStepCount; }

    private:
        b2WorldId*          m_World;
        Math::Float2        m_Gravity;
        const int           m_SubStepCount;

        List<PhysicsShape*> m_Shapes;
    };
}