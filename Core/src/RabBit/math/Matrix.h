#pragma once

#include "Core.h"
#include "Vector.h"

namespace RB::Math
{
    struct Float3x3;

    struct Float4x4
    {
    public:
        union
        {
            struct // Row major
            {
                float a[16];
            };

            struct
            {
                Float4 row[4];
            };

            struct
            {
                Float4 row0;
                Float4 row1;
                Float4 row2;
                Float4 row3;
            };

            struct // Row major
            {
                struct
                {
                    float a00, a01, a02, a03;
                };

                struct
                {
                    float a10, a11, a12, a13;
                };

                struct
                {
                    float a20, a21, a22, a23;
                };

                struct
                {
                    float a30, a31, a32, a33;
                };
            };
        };

        Float4x4();
        ~Float4x4() = default;

        void ToData(float* out);

        Float4x4 operator*(const Float4x4& other);

        void Transpose();

        Float3 GetPosition();

        void RotateAroundX(float xrad);
        void RotateAroundY(float yrad);
        void RotateAroundZ(float zrad);

        void SetPosition(const Float3 pos);
        void SetPosition(float x, float y, float z);

        void Scale(const Float3 scale);
        void Scale(float scale);
        void Scale(float x, float y, float z);

        bool Invert();
        float GetDeterminant() const;
        void GetCofactor(Float3x3& temp, int p, int q) const;
        Float4x4 GetAdjugate() const;
    };

    struct Float3x3
    {
    public:
        union
        {
            struct // Row major
            {
                float a[9];
            };

            struct
            {
                Float3 row[3];
            };

            struct
            {
                Float3 row0;
                Float3 row1;
                Float3 row2;
            };

            struct // Row major
            {
                struct
                {
                    float a00, a01, a02;
                };

                struct
                {
                    float a10, a11, a12;
                };

                struct
                {
                    float a20, a21, a22;
                };
            };
        };

        Float3x3();
        ~Float3x3() = default;

        Float3x3 operator*(const Float3x3& other);

        float GetDeterminant() const;
    };
}