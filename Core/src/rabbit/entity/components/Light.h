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
            Graphics::Frustum frustum;

            // TODO: Properly calculate the light-space bounding boxes of each CSM slice

            Math::Float3 dir = m_Direction;

            dir.y = Math::Clamp(dir.y, -1.0f, 1.0f);

            // yaw: rotate around Y so +Z aligns with projection of dir on XZ plane
            float yaw = atan2(dir.x, dir.z);   // atan2(y, x) -> here y=dir.x, x=dir.z

            // pitch: rotation around X to raise/lower from forward
            float pitch = -asin(dir.y);        // note the minus to match convention used earlier

            float roll = 0.0f; // cannot be derived from direction alone

            Math::Float3 rot = Math::Float3(Math::RadiansToDegrees(pitch), Math::RadiansToDegrees(yaw), Math::RadiansToDegrees(roll));

            Math::Float3 light_pos = Math::Float3(0, 25, 0); //cam_transform.position; //Math::Float3(0, -250, 0);
            
            //Graphics::Frustum frustum;
            frustum.SetTransform(light_pos, rot);
            frustum.SetOrthographicProjection(0.01f, 1000.0f, -100, 100, 100, -100, false);
            //frustum.SetPerspectiveProjection(0.01f, 1000.0f, -1, 1, 1, -1, false);

            return frustum;
        }

    private:
        Math::Float3        m_Direction;
        Math::Float3        m_Color;
    };
}