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

        Graphics::Frustum CalculateFrustum(const Camera& camera, const Transform& cam_transform, uint32_t slice, uint32_t total_slices) const
        {
            Math::Float3 dir = m_Direction;

            dir.y = Math::Clamp(dir.y, -1.0f, 1.0f);

            // yaw: rotate around Y so +Z aligns with projection of dir on XZ plane
            const float yaw = atan2(dir.x, dir.z);

            // pitch: rotation around X to raise/lower from forward
            const float pitch = -asin(dir.y);

            const Math::Float3 rot = Math::Float3(Math::RadiansToDegrees(pitch), Math::RadiansToDegrees(yaw), 0.0f);

            float cam_pitch = Math::DegreesToRadians(-cam_transform.rotation.x);
            float cam_yaw   = Math::DegreesToRadians(cam_transform.rotation.y);

            const Math::Float3 cam_forward(
                Math::Cos(cam_pitch) * Math::Sin(cam_yaw),
                Math::Sin(cam_pitch),
                Math::Cos(cam_pitch) * Math::Cos(cam_yaw)
            );

            const float frustum_distance = (camera.GetFarPlane() * m_ShadowDistanceCoverage) * Math::Pow((float)(slice + 1) / (float)total_slices, m_SliceSteepness);
            const float frustum_far = 5000.0f; // Just use a big value so we don't clip into any big objects

            Math::Float3 light_pos = cam_transform.position;
            // Nudge the position forward with quarter the frustum distance
            light_pos = light_pos + cam_forward * (frustum_distance * 0.25f);
            // Move the light up by 80% of the far distance
            light_pos = light_pos + dir * (frustum_far * -0.8f);
            
            Graphics::Frustum frustum;
            frustum.SetTransform(light_pos, rot);
            frustum.SetOrthographicProjection(0.1f, frustum_far, -frustum_distance, frustum_distance, frustum_distance, -frustum_distance, false);

            return frustum;
        }

    private:
        Math::Float3        m_Direction;
        Math::Float3        m_Color;

        float               m_SliceSteepness         = 2.8f; // How fast do we transition to the next shadow slice? (the higher the less distance the first few slices will cover) 
        float               m_ShadowDistanceCoverage = 0.7f; // The percentage of the camera frustum the shadow maps will cover
    };
}