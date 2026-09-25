#include "RabBitCommon.h"
#include "Quaternion.h"
#include "Misc.h"

namespace RB::Math
{
    Quaternion::Quaternion()
        : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
    {
    }

    Quaternion::Quaternion(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {
    }

    Quaternion Quaternion::Identity()
    {
        return Quaternion();
    }

    float Quaternion::GetLength() const
    {
        return Sqrt(x * x + y * y + z * z + w * w);
    }

    void Quaternion::Normalize()
    {
        float length = GetLength();

        if (length == 0.0f)
        {
            *this = Quaternion();
            return;
        }

        float inv = 1.0f / length;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
    }

    Quaternion Quaternion::GetConjugate() const
    {
        return Quaternion(-x, -y, -z, w);
    }

    Quaternion Quaternion::GetInverse() const
    {
        float len_sq = x * x + y * y + z * z + w * w;

        if (len_sq == 0.0f)
        {
            return Quaternion();
        }

        float inv = 1.0f / len_sq;
        return Quaternion(-x * inv, -y * inv, -z * inv, w * inv);
    }

    Quaternion Quaternion::operator*(const Quaternion& o) const
    {
        return Quaternion(
            o.w * x + o.x * w + o.y * z - o.z * y,
            o.w * y - o.x * z + o.y * w + o.z * x,
            o.w * z + o.x * y - o.y * x + o.z * w,
            o.w * w - o.x * x - o.y * y - o.z * z
        );
    }

    Float3 Quaternion::Rotate(const Float3& v) const
    {
        Float3 q(x, y, z);
        Float3 t = Float3::Cross(q, v) * 2.0f;
        return v + Float3::Cross(q, t) + t * w;
    }

    Float4x4 Quaternion::ToMatrix() const
    {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        Float4x4 m;
        m.row0 = Float4(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
        m.row1 = Float4(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
        m.row2 = Float4(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
        m.row3 = Float4(0.0f, 0.0f, 0.0f, 1.0f);
        return m;
    }

    Quaternion Quaternion::FromAxisAngle(const Float3& axis, float radians)
    {
        Float3 n = axis;
        n.Normalize();

        float half = radians * 0.5f;
        float s = Sin(half);

        return Quaternion(n.x * s, n.y * s, n.z * s, Cos(half));
    }

    Quaternion Quaternion::FromEuler(float x_rad, float y_rad, float z_rad)
    {
        Quaternion qx = FromAxisAngle(WorldRight, x_rad);
        Quaternion qy = FromAxisAngle(WorldUp, y_rad);
        Quaternion qz = FromAxisAngle(WorldForward, z_rad);

        // X first, then Y, then Z (same as calling RotateAroundX, Y, Z in that order)
        return qx * qy * qz;
    }

    Quaternion Quaternion::FromEuler(const Float3& euler_rad)
    {
        return FromEuler(euler_rad.x, euler_rad.y, euler_rad.z);
    }

    // Inverse of FromEuler (X, then Y, then Z). Angles are not unique, so
    // ToEuler(FromEuler(e)) can return a different but equivalent triple.
    Float3 Quaternion::ToEuler() const
    {
        float m00 = 1.0f - 2.0f * (y * y + z * z);
        float m01 = 2.0f * (x * y + z * w);
        float m02 = 2.0f * (x * z - y * w);
        float m12 = 2.0f * (y * z + x * w);
        float m22 = 1.0f - 2.0f * (x * x + y * y);

        Float3 e;
        e.y = ArcSin(Clamp(-m02, -1.0f, 1.0f));

        if (Abs(m02) < 0.99999f)
        {
            e.x = ArcTan2(m12, m22);
            e.z = ArcTan2(m01, m00);
        }
        else
        {
            // Gimbal lock: x and z rotate around the same axis, pick z = 0
            float m11 = 1.0f - 2.0f * (x * x + z * z);
            float m21 = 2.0f * (y * z - x * w);
            e.x = ArcTan2(-m21, m11);
            e.z = 0.0f;
        }

        return e;
    }

    Quaternion Quaternion::FromMatrix(const Float4x4& m)
    {
        float trace = m.a00 + m.a11 + m.a22;
        Quaternion q;

        if (trace > 0.0f)
        {
            float s = Sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (m.a12 - m.a21) / s;
            q.y = (m.a20 - m.a02) / s;
            q.z = (m.a01 - m.a10) / s;
        }
        else if (m.a00 > m.a11 && m.a00 > m.a22)
        {
            float s = Sqrt(1.0f + m.a00 - m.a11 - m.a22) * 2.0f;
            q.w = (m.a12 - m.a21) / s;
            q.x = 0.25f * s;
            q.y = (m.a01 + m.a10) / s;
            q.z = (m.a02 + m.a20) / s;
        }
        else if (m.a11 > m.a22)
        {
            float s = Sqrt(1.0f + m.a11 - m.a00 - m.a22) * 2.0f;
            q.w = (m.a20 - m.a02) / s;
            q.x = (m.a01 + m.a10) / s;
            q.y = 0.25f * s;
            q.z = (m.a12 + m.a21) / s;
        }
        else
        {
            float s = Sqrt(1.0f + m.a22 - m.a00 - m.a11) * 2.0f;
            q.w = (m.a01 - m.a10) / s;
            q.x = (m.a02 + m.a20) / s;
            q.y = (m.a12 + m.a21) / s;
            q.z = 0.25f * s;
        }

        q.Normalize();
        return q;
    }

    float Quaternion::Dot(const Quaternion& a, const Quaternion& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    Quaternion Quaternion::Lerp(const Quaternion& from, const Quaternion& to, float t)
    {
        // q and -q are the same rotation, take the short way around
        float sign = Dot(from, to) < 0.0f ? -1.0f : 1.0f;

        Quaternion out(
            Math::Lerp(from.x, to.x * sign, t),
            Math::Lerp(from.y, to.y * sign, t),
            Math::Lerp(from.z, to.z * sign, t),
            Math::Lerp(from.w, to.w * sign, t)
        );
        out.Normalize();
        return out;
    }

    Quaternion Quaternion::Slerp(const Quaternion& from, const Quaternion& to, float t)
    {
        float d = Dot(from, to);
        Quaternion target = to;

        if (d < 0.0f)
        {
            d = -d;
            target = Quaternion(-to.x, -to.y, -to.z, -to.w);
        }

        // Nearly parallel: sin(theta) approaches 0, fall back to nlerp
        if (d > 0.9995f)
        {
            return Lerp(from, target, t);
        }

        float theta = ArcCos(d);
        float inv_sin = 1.0f / Sin(theta);
        float wa = Sin((1.0f - t) * theta) * inv_sin;
        float wb = Sin(t * theta) * inv_sin;

        return Quaternion(
            from.x * wa + target.x * wb,
            from.y * wa + target.y * wb,
            from.z * wa + target.z * wb,
            from.w * wa + target.w * wb
        );
    }

    Quaternion Quaternion::LookRotation(const Float3& forward, const Float3& up)
    {
        Float3 f = forward;
        f.Normalize();

        Float3 axis = Float3::Cross(WorldForward, f);
        float axis_len = axis.GetLength();
        float dot = Math::Clamp(Float3::Dot(WorldForward, f), -1.0f, 1.0f);

        Quaternion q1;
        if (axis_len < 1e-6f)
        {
            // forward is parallel or anti-parallel to identity forward
            q1 = (dot > 0.0f) ? Quaternion::Identity() : Quaternion::FromAxisAngle(WorldUp, kPI);
        }
        else
        {
            axis = axis / axis_len;
            float angle = ArcCos(dot);
            q1 = Quaternion::FromAxisAngle(axis, angle);
        }

        Float3 rotated_up = q1.Rotate(WorldUp);
        Float3 desired_up = up - f * Float3::Dot(up, f); // project out component along f
        desired_up.Normalize();

        float roll_dot = Math::Clamp(Float3::Dot(rotated_up, desired_up), -1.0f, 1.0f);
        float roll_angle = ArcCos(roll_dot);

        Float3 roll_cross = Float3::Cross(rotated_up, desired_up);
        if (Float3::Dot(roll_cross, f) < 0.0f)
        {
            roll_angle = -roll_angle;
        }

        Quaternion q2 = Quaternion::FromAxisAngle(f, roll_angle);
        return q1 * q2;
    }
}