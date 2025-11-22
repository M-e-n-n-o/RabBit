#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "entity/components/Camera.h"
#include "entity/components/Transform.h"
#include "graphics/Frustum.h"

namespace RB::Entity
{
    class DirectionalLight : public ObjectComponent
    {
    public:
        DirectionalLight(Math::Float3 direction, Math::Float3 color)
            : m_Direction(direction)
            , m_Color(color)
        {
            m_Direction.Normalize();
        }

        Math::Float3 GetDirection() const { return m_Direction; }
        Math::Float3 GetColor() const { return m_Color; }

        Graphics::Frustum CalculateFrustum(const Camera& camera, const Transform& cam_transform) const
        {
            // TODO: Properly calculate the light-space bounding boxes of each CSM slice

            Math::Float3 dir = m_Direction;

            dir.y = Math::Clamp(dir.y, -1.0f, 1.0f);

            // yaw: rotate around Y so +Z aligns with projection of dir on XZ plane
            float yaw = atan2(dir.x, dir.z);

            // pitch: rotation around X to raise/lower from forward
            float pitch = -asin(dir.y);

            Math::Float3 rot = Math::Float3(Math::RadiansToDegrees(pitch), Math::RadiansToDegrees(yaw), 0.0f);

            Math::Float3 light_pos = cam_transform.position + dir * -750.0f;
            
            Graphics::Frustum frustum;
            frustum.SetTransform(light_pos, rot);
            frustum.SetOrthographicProjection(0.01f, 1000.0f, -10, 10, 10, -10, false);

            return frustum;
        }

    private:
        Math::Float3        m_Direction;
        Math::Float3        m_Color;
    };
}