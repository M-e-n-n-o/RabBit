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

        Graphics::Frustum CalculateFrustum(const Camera& camera, const Transform& cam_transform, uint32_t slice, uint32_t total_slices, float* out_split) const
        {
            // Calculate Split Distances
            float near_p = Math::Max(camera.GetNearPlane(), 0.5f);
            float far_p = Math::Min(camera.GetFarPlane(), m_ShadowDistance);
            float ratio = far_p / near_p;

            // Practical split lerp between logarithmic and linear
            float p = (float)(slice + 1) / (float)total_slices;
            float log_split = near_p * Math::Pow(ratio, p);
            float lin_split = near_p + (far_p - near_p) * p;
            float split_dist = Math::Lerp(log_split, lin_split, m_SliceSteepness);

            float prev_split = (slice == 0) ? camera.GetNearPlane() : *out_split;
            *out_split = split_dist;

            // Get the corners of this frustum slice in world space
            Graphics::Frustum temp_frustum;
            temp_frustum.SetTransform(cam_transform.position, cam_transform.rotation);
            temp_frustum.SetPerspectiveProjectionVFov(prev_split, split_dist,
                                                      camera.GetVerticalFovInRadians(), 
                                                      ((float)camera.GetRenderTargetWidth() / (float)camera.GetRenderTargetHeight()),
                                                      false);

            Math::Float4x4 slice_view_proj = temp_frustum.GetWorldToViewMatrix() * temp_frustum.GetViewToClipMatrix();
            Math::Float4x4 inv_slice_vp = slice_view_proj;
            inv_slice_vp.Invert();

            List<Math::Float3> corners = Graphics::Frustum::GetFrustumCornersWorldSpace(inv_slice_vp);

            // Create the Light View Matrix
            Math::Float3 center(0.0f, 0.0f, 0.0f);
            for (const auto& v : corners) {
                center = center + v;
            }
            center = center * (1.0f / 8.0f);

            Math::Float3 lightPos = center - (m_Direction * m_PullBackDistance);

            // The light looks at the center of our frustum slice
            temp_frustum.LookAt(lightPos, center, Math::Float3(0, 1, 0));
            Math::Float4x4 lightView = temp_frustum.GetWorldToViewMatrix();

            // Find Min/Max in Light Space
            float min_x = FLT_MAX, max_x = -FLT_MAX;
            float min_y = FLT_MAX, max_y = -FLT_MAX;
            float max_z = -FLT_MAX;

            for (const auto& v : corners) 
            {
                Math::Float3 trf = v * lightView;
                min_x = Math::Min(min_x, trf.x); max_x = Math::Max(max_x, trf.x);
                min_y = Math::Min(min_y, trf.y); max_y = Math::Max(max_y, trf.y);
                max_z = Math::Max(max_z, trf.z);
            }

            // Build the Final Frustum
            Graphics::Frustum frustum;
            frustum.SetTransform(lightView);
            frustum.SetOrthographicProjection(0.1f, max_z + 25.0f, min_x, max_x, max_y, min_y, false);
            return frustum;
        }

    private:
        Math::Float3        m_Direction;
        Math::Float3        m_Color;

        float               m_SliceSteepness = 0.4f;     // How fast do we transition to the next shadow slice?
        float               m_ShadowDistance = 500.0f;   // The max shadow coverage
        float               m_PullBackDistance = 900.0f; // How far away is the directionalLight from the scene?
    };
}