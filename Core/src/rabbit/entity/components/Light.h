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
            //float        cam_near = camera.GetNearPlane();
            //float        cam_far  = camera.GetFarPlane();
            //float        cam_vfov = camera.GetVerticalFovInRadians();
            //float        cam_asp  = (float)camera.GetRenderTargetWidth() / (float)camera.GetRenderTargetHeight();
            //Math::Float3 cam_pos  = cam_transform.GetWorldPosition();
            //Math::Float3 cam_dir  = cam_transform.GetWorldRotation();
            //
            //
            //float half_height = tanf(cam_vfov * 0.5f) * cam_far;
            //float half_width = half_height * cam_asp;
            //
            //// Radius is diagonal of far-plane rectangle
            //float radius = sqrtf(half_width * half_width + half_height * half_height);
            //
            //Math::Float3 target = cam_pos + cam_dir * (cam_far * 0.5f);
            //
            //// Choose a distance behind the light
            //float distance_back = radius;  // usually bounding sphere radius
            //
            //// Light position
            //Math::Float3 light_pos = target - m_Direction * distance_back;
            //
            Graphics::Frustum frustum;
            //frustum.LookAt(light_pos, target, Math::WorldUp);
            //
            //
            //Math::Float3 center = cam_pos + cam_dir * (cam_far * 0.5f);
            //
            //// Move the sphere center into light-view space
            //Math::Float4 centerLS4 = frustum.GetWorldToViewMatrix() * Math::Float4(center.x, center.y, center.z, 1.0f);
            //Math::Float3 centerLS(centerLS4.x, centerLS4.y, centerLS4.z);
            //
            //frustum.SetOrthographicProjection(centerLS.z - radius, centerLS.z + radius,
            //                                  centerLS.x - radius, centerLS.x + radius,
            //                                  centerLS.y - radius, centerLS.y + radius, 
            //                                  false);


            // TODO: Properly calculate the light-space bounding boxes of each CSM slice
            float distance_back = 150.0f;
            Math::Float3 light_pos = Math::Float3(0, 0, 0) - m_Direction * distance_back;
            
            //Graphics::Frustum frustum;
            frustum.SetTransform(light_pos, m_Direction);
            frustum.SetOrthographicProjection(0.01f, 10000.0f, -250, 250, -250, 250, false);

            return frustum;
        }

    private:
        Math::Float3        m_Direction;
        Math::Float3        m_Color;
    };
}