#pragma once

#include "Core.h"

namespace RB::Math
{
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

        float GetDeterminant() const;
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

    // Function to get the cofactor of element (p, q)
    void MatGetCofactor(Float4x4 mat, Float3x3& temp, int p, int q) 
    {
        int i = 0, j = 0;
        for (int row = 0; row < 4; row++) 
        {
            for (int col = 0; col < 4; col++) 
            {
                if (row != p && col != q) 
                {
                    temp.a[(i + 1) * j] = mat.a[(row + 1) * col];
                    j++;
                    if (j == 3) 
                    {
                        j = 0;
                        i++;
                    }
                }
            }
        }
    }

    // Function to compute adjugate
    void MatAdjugate(Float4x4 mat, Float4x4 adj) {
        Float3x3 temp;
        int sign;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                getCofactor(mat, temp, i, j);
                sign = ((i + j) % 2 == 0) ? 1 : -1;
                adj[j][i] = sign * det3x3(temp); // Transpose!
            }
        }
    }

    // Function to invert the matrix
    bool MatInvert(Float4x4 mat, Float4x4 inv) {
        float det = MatDeterminant(mat);
        if (det == 0) return false;

        float adj[4][4];
        adjugate(mat, adj);

        // Divide adjugate by determinant
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                inv[i][j] = adj[i][j] / det;

        return true;
    }
}