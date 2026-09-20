#pragma once

#include "Core.h"
#include "Vector.h"
#include "Matrix.h"

namespace RB::Math
{
    // Unit quaternion representing a rotation.
    //
    // Conventions (matches Float4x4):
    //  - Same rotation direction as Float4x4::RotateAroundX/Y/Z for a positive angle.
    //  - Composition reads left to right, exactly like the row-vector matrices:
    //        (a * b) means "apply a first, then b"
    //    so  (a * b).ToMatrix() == a.ToMatrix() * b.ToMatrix()
    //    and v * (a * b).ToMatrix() == (a * b).Rotate(v)
    struct Quaternion
    {
    public:
        float x, y, z, w;

        Quaternion(); // Identity
        Quaternion(float x, float y, float z, float w);
        ~Quaternion() = default;

        void Normalize();
        float GetLength() const;
        Quaternion GetConjugate() const;
        Quaternion GetInverse() const;

        Quaternion operator*(const Quaternion& other) const;

        Float3 Rotate(const Float3& v) const;                   // Rotates a direction / point around the origin
        Float4x4 ToMatrix() const;                              // Rotation only (row 3 = 0,0,0,1)
        Float3 ToEuler() const;                                 // Radians (x, y, z), see FromEuler for the order

        static Quaternion Identity();
        static Quaternion FromAxisAngle(const Float3& axis, float radians);

        // Same result as: m.RotateAroundX(x); m.RotateAroundY(y); m.RotateAroundZ(z);
        static Quaternion FromEuler(float x_rad, float y_rad, float z_rad);
        static Quaternion FromEuler(const Float3& euler_rad);

        // Matrix must be rotation only (no scale). Normalize the rows first if it has scale.
        static Quaternion FromMatrix(const Float4x4& m);

        static float Dot(const Quaternion& first, const Quaternion& second);
        static Quaternion Nlerp(const Quaternion& from, const Quaternion& to, float t);
        static Quaternion Slerp(const Quaternion& from, const Quaternion& to, float t);
    };
}