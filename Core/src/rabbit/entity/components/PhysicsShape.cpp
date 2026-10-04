#include "RabBitCommon.h"
#include "PhysicsShape.h"
#include "entity/GameObject.h"
#include "entity/components/Transform.h"
#include "entity/components/PhysicsWorld.h"

#include <box2d/box2d.h>

namespace RB::Entity
{
    float QuaternionTo2DAngle(const Math::Quaternion& q)
    {
        // Extract the Z-axis rotation angle from the quaternion
        float sin_z = 2.0f * (q.w * q.z + q.x * q.y);
        float cos_z = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);

        float angle_rad = Math::ArcTan2(sin_z, cos_z);
        return angle_rad;
    }

    PhysicsBox2D::PhysicsBox2D(const Math::Float2& size, bool is_kinematic, PhysicsWorld2D* world)
        : m_World(world)
        , m_Size(size)
        , m_Kinematic(is_kinematic)
        , m_PrevPos()
        , m_PrevAngle(0.0f)
    {
    }
    
    PhysicsBox2D::~PhysicsBox2D()
    {
        m_World->Unregister(this);

        b2DestroyShape(*m_ShapeID, true);
        delete m_ShapeID;

        b2DestroyBody(*m_BodyID);
        delete m_BodyID;
    }

    void PhysicsBox2D::OnAttached()
    {
        m_Transform = m_GameObject->GetComponent<Transform>();
        RB_ASSERT(LOGTAG_ENTITY, m_Transform != nullptr, "PhysicsBox2D requires a transform");

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type       = m_Kinematic ? b2_kinematicBody : b2_dynamicBody;
        body_def.position   = { m_Transform->GetPositionX(), m_Transform->GetPositionY() };
        body_def.rotation   = { b2MakeRot(QuaternionTo2DAngle(m_Transform->GetRotation())) };

        m_BodyID = new b2BodyId();
        *m_BodyID = b2CreateBody(*m_World->GetWorldID(), &body_def);

        b2Polygon box = b2MakeBox(m_Size.x / 2.0f, m_Size.y / 2.0f);

        b2ShapeDef shape_def = b2DefaultShapeDef();

        m_ShapeID = new b2ShapeId();
        *m_ShapeID = b2CreatePolygonShape(*m_BodyID, &shape_def, &box);

        m_World->Register(this);
    }

    void PhysicsBox2D::PrePhysicsStep()
    {
        b2Vec2 position = b2Body_GetPosition(*m_BodyID);
        b2Rot rotation = b2Body_GetRotation(*m_BodyID);

        m_PrevPos = { position.x, position.y };
        m_PrevAngle = b2Rot_GetAngle(rotation);
    }
    
    void PhysicsBox2D::PostPhysicsStep(float alpha)
    {
        b2Vec2 position  = b2Body_GetPosition(*m_BodyID);
        b2Rot  rotation  = b2Body_GetRotation(*m_BodyID);
        float  new_angle = b2Rot_GetAngle(rotation);

        float x = Math::Lerp(m_PrevPos.x, position.x, alpha);
        float y = Math::Lerp(m_PrevPos.y, position.y, alpha);

        float delta = new_angle - m_PrevAngle;
        while (delta > kPI) 
            delta -= 2.0f * kPI;
        while (delta < -kPI)
            delta += 2.0f * kPI;

        float angle = Math::Lerp(m_PrevAngle, m_PrevAngle + delta, alpha);

        m_Transform->SetPositionX(x);
        m_Transform->SetPositionY(y);
        m_Transform->SetRotation(Math::Quaternion::FromAxisAngle(Math::WorldForward, angle));
    }
}
