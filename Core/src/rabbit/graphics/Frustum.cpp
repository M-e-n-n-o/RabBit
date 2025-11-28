#include "RabBitCommon.h"
#include "Frustum.h"

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

namespace RB::Graphics
{
    Frustum::Frustum()
        : m_ReversedDepth(false)
    {
    }

    void Frustum::SetTransform(Math::Float3 position, Math::Float3 rotation)
    {
        Math::Float4x4 m;

        m.RotateAroundX(Math::DegreesToRadians(rotation.x));
        m.RotateAroundY(Math::DegreesToRadians(rotation.y));
        m.RotateAroundZ(Math::DegreesToRadians(rotation.z));
        
        m.SetPosition(position);
        m.Scale(1.0f);
        
        m.Invert();

        m_WorldToViewMat = m;
    }

    void Frustum::SetPerspectiveProjectionVFov(float near, float far, float vfov, float aspect, bool reverse_depth)
    {
        float ty = Math::Tan(vfov * 0.5f);
        float tx = ty * aspect;

        SetPerspectiveProjection(near, far, -tx, tx, ty, -ty, reverse_depth);
    }

    void Frustum::SetPerspectiveProjection(float near, float far, float left, float right, float top, float bottom, bool reverse_depth)
    {
        if (far > kFarClipMax)
        {
            far = kFarClipMax;
        }

        if (near <= 0.0f)
        {
            near = far / (kFarClipMax * 10.0f);
        }

        m_ViewToClipMat = Math::Float4x4();
        m_ViewToClipMat.row0 = Math::Float4(2.0f / (right - left),              0,                                  0,                              0);
        m_ViewToClipMat.row1 = Math::Float4(0,                                  2.0f / (top - bottom),              0,                              0);
        m_ViewToClipMat.row2 = Math::Float4((right + left) / (left - right),    (top + bottom) / (bottom - top),    far / (far - near),             1);
        m_ViewToClipMat.row3 = Math::Float4(0,                                  0,                                  (far * near) / (near - far),    0);

        if (reverse_depth)
        {
            m_ViewToClipMat.a22 = near / (near - far);
            m_ViewToClipMat.a32 = (near * far) / (far - near);
        }

        // Horizontal fov in radians
        m_HFov = Math::ArcTan2(right, 1.0f) - Math::ArcTan2(left, 1.0f);

        // Vertical fov in radians
        m_VFov = Math::ArcTan2(bottom, 1.0f) - Math::ArcTan2(top, 1.0f);

        m_AspectRatio = (right - left) / (bottom - top);

        m_ViewLength = far - near;

        m_ReversedDepth = reverse_depth;
    }

    void Frustum::SetOrthographicProjection(float near, float far, float left, float right, float top, float bottom, bool reverse_depth)
    {
        if (far > kFarClipMax)
        {
            far = kFarClipMax;
        }

        if (near <= 0.0f)
        {
            near = far / (kFarClipMax * 10.0f);
        }

        m_ViewToClipMat = Math::Float4x4();
        m_ViewToClipMat.row0 = Math::Float4(2.0f / (right - left),              0,                                  0,                      0);
        m_ViewToClipMat.row1 = Math::Float4(0,                                  2.0f / (top - bottom),              0,                      0);
        m_ViewToClipMat.row2 = Math::Float4(0,                                  0,                                  1.0f / (far - near),    0);
        m_ViewToClipMat.row3 = Math::Float4((left + right) / (left - right),    (top + bottom) / (bottom - top),    -near / (far - near),   1);

        if (reverse_depth)
        {
            m_ViewToClipMat.a22 = 1.0f / (near - far);
            m_ViewToClipMat.a32 = near / (near - far);
        }

        m_HFov = 0.0f;
        m_VFov = 0.0f;

        m_AspectRatio = (right - left) / (bottom - top);
        m_ViewLength = far - near;

        m_ReversedDepth = reverse_depth;
    }

    bool Frustum::IsInFrustum(const Math::AABB& aabb, const Math::Float4x4& view_proj)
    {
        // Add "small" padding to fix early culling of small objects
        float epsilon = 0.1f;
        Math::Float3 padded_min = aabb.min - epsilon;
        Math::Float3 padded_max = aabb.max + epsilon;
        
        const float* m = view_proj.a;

        // For each plane, test the "positive vertex" of the AABB
        for (int i = 0; i < 6; ++i)
        {
            // Compute plane coefficients a,b,c,d depending on plane
            Math::Float3 plane;
            float d_plane;
            switch (i)
            {
            case 0: // Left
                plane.x = m[3] + m[0];
                plane.y = m[7] + m[4];
                plane.z = m[11] + m[8];
                d_plane = m[15] + m[12];
                break;
            case 1: // Right
                plane.x = m[3] - m[0];
                plane.y = m[7] - m[4];
                plane.z = m[11] - m[8];
                d_plane = m[15] - m[12];
                break;
            case 2: // Bottom
                plane.x = m[3] + m[1];
                plane.y = m[7] + m[5];
                plane.z = m[11] + m[9];
                d_plane = m[15] + m[13];
                break;
            case 3: // Top
                plane.x = m[3] - m[1];
                plane.y = m[7] - m[5];
                plane.z = m[11] - m[9];
                d_plane = m[15] - m[13];
                break;
            case 4: // Near
                plane.x = m[3] + m[2];
                plane.y = m[7] + m[6];
                plane.z = m[11] + m[10];
                d_plane = m[15] + m[14];
                break;
            case 5: // Far
                plane.x = m[3] - m[2];
                plane.y = m[7] - m[6];
                plane.z = m[11] - m[10];
                d_plane = m[15] - m[14];
                break;
            }

            // Normalize
            float length = plane.GetLength();
            plane = plane / length;
            d_plane /= length;

            // Compute positive vertex of AABB for this plane
            Math::Float3 p = padded_min;
            if (plane.x >= 0) p.x = padded_max.x;
            if (plane.y >= 0) p.y = padded_max.y;
            if (plane.z >= 0) p.z = padded_max.z;

            // If positive vertex is outside the plane, AABB is outside frustum
            if ((plane.x * p.x + plane.y * p.y + plane.z * p.z + d_plane) < 0)
                return false;
        }

        return true;
    }
}