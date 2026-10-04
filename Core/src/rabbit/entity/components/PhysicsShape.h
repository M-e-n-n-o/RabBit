#pragma once
#include "entity/ObjectComponent.h"

struct b2BodyId;
struct b2ShapeId;

namespace RB::Entity
{
    class Transform;
    class PhysicsWorld2D;

    class PhysicsShape : public ObjectComponent
    {
    public:
        virtual ~PhysicsShape() = default;

        virtual void PrePhysicsStep() = 0;
        virtual void PostPhysicsStep(float alpha) = 0;
    };

    class PhysicsBox2D : public PhysicsShape
    {
    public:
        PhysicsBox2D(const Math::Float2& size, bool is_kinematic, PhysicsWorld2D* world);
        ~PhysicsBox2D();

        void OnAttached() override;

        void PrePhysicsStep() override;
        void PostPhysicsStep(float alpha) override;

    private:
        Transform*      m_Transform;
        PhysicsWorld2D* m_World;
        Math::Float2    m_Size;
        bool            m_Kinematic;

        b2BodyId*       m_BodyID;
        b2ShapeId*      m_ShapeID;
        Math::Float2    m_PrevPos;
        float           m_PrevAngle;
    };
    REGISTER_COMP_BASES(PhysicsBox2D, PhysicsShape);
}