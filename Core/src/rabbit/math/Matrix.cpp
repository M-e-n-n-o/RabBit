#include "RabBitCommon.h"
#include "Matrix.h"

namespace RB::Math
{
    // ---------------------------------------------------------------------------
    //								    Float4x4
    // ---------------------------------------------------------------------------

    Float4x4::Float4x4()
    {
        row0 = { 1, 0, 0, 0 };
        row1 = { 0, 1, 0, 0 };
        row2 = { 0, 0, 1, 0 };
        row3 = { 0, 0, 0, 1 };
    }

    Float4x4::Float4x4(float v)
    {
        row0 = { v, v, v, v };
        row1 = { v, v, v, v };
        row2 = { v, v, v, v };
        row3 = { v, v, v, v };
    }

    void Float4x4::ToData(float* out)
    {
        memcpy(out, a, 16 * sizeof(float));
    }

    Float4x4 Float4x4::operator*(const Float4x4& other)
    {
        Float4x4 out;
        out.row0 = (other.row0 * row0.x) + (other.row1 * row0.y) + (other.row2 * row0.z) + (other.row3 * row0.w);
        out.row1 = (other.row0 * row1.x) + (other.row1 * row1.y) + (other.row2 * row1.z) + (other.row3 * row1.w);
        out.row2 = (other.row0 * row2.x) + (other.row1 * row2.y) + (other.row2 * row2.z) + (other.row3 * row2.w);
        out.row3 = (other.row0 * row3.x) + (other.row1 * row3.y) + (other.row2 * row3.z) + (other.row3 * row3.w);
        return out;
    }

    void Float4x4::Transpose()
    {
        Float4x4 copy = *this;

        row0 = { copy.row0.x, copy.row1.x, copy.row2.x, copy.row3.x };
        row1 = { copy.row0.y, copy.row1.y, copy.row2.y, copy.row3.y };
        row2 = { copy.row0.z, copy.row1.z, copy.row2.z, copy.row3.z };
        row3 = { copy.row0.w, copy.row1.w, copy.row2.w, copy.row3.w };
    }

    void Float4x4::RotateAroundX(float xrad)
    {
        float s = Sin(xrad);
        float c = Cos(xrad);
    
        // Create rotation matrix
        Float4x4 rot;
        rot.row0 = Float4(1, 0, 0, 0);
        rot.row1 = Float4(0, c, s, 0);
        rot.row2 = Float4(0, -s, c, 0);
        rot.row3 = Float4(0, 0, 0, 1);
    
        // Apply rotation
        *this = (*this) * rot;
    }
    
    void Float4x4::RotateAroundY(float yrad)
    {
        float s = Sin(yrad);
        float c = Cos(yrad);
    
        // Create rotation matrix
        Float4x4 rot;
        rot.row0 = Float4(c, 0, -s, 0);
        rot.row1 = Float4(0, 1, 0, 0);
        rot.row2 = Float4(s, 0, c, 0);
        rot.row3 = Float4(0, 0, 0, 1);
    
        // Apply rotation
        *this = (*this) * rot;
    }
    
    void Float4x4::RotateAroundZ(float zrad)
    {
        float s = Sin(zrad);
        float c = Cos(zrad);
    
        // Create rotation matrix
        Float4x4 rot;
        rot.row0 = Float4(c, s, 0, 0);
        rot.row1 = Float4(-s, c, 0, 0);
        rot.row2 = Float4(0, 0, 1, 0);
        rot.row3 = Float4(0, 0, 0, 1);
    
        // Apply rotation
        *this = (*this) * rot;
    }

    Float3 Float4x4::GetPosition()
    {
        return Float3(a30, a31, a32);
    }

    void Float4x4::SetPosition(const Float3& pos)
    {
        SetPosition(pos.x, pos.y, pos.z);
    }

    void Float4x4::SetPosition(float x, float y, float z)
    {
        a30 = x;
        a31 = y;
        a32 = z;
        a33 = 1;
    }

    void Float4x4::Scale(const Float3& scale)
    {
        Scale(scale.x, scale.y, scale.z);
    }

    void Float4x4::Scale(float scale)
    {
        Scale(scale, scale, scale);
    }

    void Float4x4::Scale(float x, float y, float z)
    {
        a00 *= x;
        a11 *= y;
        a22 *= z;
    }

    // Specialized, faster invert for projection matrices
    void Float4x4::InvertProjection()
    {
        Float4x4 inv = {};

        // Top-left diagonal (always invert)
        inv.a[0] = 1.0f / a[0];
        inv.a[5] = 1.0f / a[5];

        float m22 = a[10];
        float m23 = a[11];
        float m32 = a[14];
        float m33 = a[15];

        const float epsilon = 1e-6f;

        if (fabs(m32) > epsilon)
        {
            // Perspective projection
            float inv_m32 = 1.0f / m32;

            inv.a[10] = -m33 * inv_m32;
            inv.a[11] = 1.0f;         
            inv.a[14] = m22 * inv_m32;
            inv.a[15] = -m23 * inv_m32;
        }
        else
        {
            // Orthographic projection
            inv.a[10] = 1.0f / m22;
            inv.a[11] = 0.0f;      
            inv.a[14] = -m23 / m22;
            inv.a[15] = 1.0f;      
        }

        // Copy back
        memcpy(a, inv.a, sizeof(float) * 16);
    }

    bool Float4x4::Invert()
    {
        float det = GetDeterminant();
        if (det == 0)
            return false;

        Float4x4 adj = GetAdjugate();

        Float4x4 inverse;

        // Divide adjugate by determinant
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                inverse.a[i * 4 + j] = adj.a[i * 4 + j] / det;

        memcpy(a, inverse.a, sizeof(float) * 16);

        return true;
    }

    float Float4x4::GetDeterminant() const
    {
        float det = 0.0f;
        Float3x3 temp;
        int sign = 1;

        for (int f = 0; f < 4; f++) 
        {
            GetCofactor(temp, 0, f);
            det += sign * a[f] * temp.GetDeterminant();
            sign = -sign;
        }
        return det;
    }

    void Float4x4::GetCofactor(Float3x3& temp, int p, int q) const
    {
        int i = 0, j = 0;
        for (int row = 0; row < 4; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                if (row != p && col != q)
                {
                    temp.a[i * 3 + j] = a[row * 4 + col];
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

    Float4x4 Float4x4::GetAdjugate() const
    {
        Float4x4 adj;
        Float3x3 temp;
        int sign;

        for (int i = 0; i < 4; i++) 
        {
            for (int j = 0; j < 4; j++) 
            {
                GetCofactor(temp, i, j);
                sign = ((i + j) % 2 == 0) ? 1 : -1;
                adj.a[j * 4 + i] = sign * temp.GetDeterminant(); // Transpose of cofactor
            }
        }

        return adj;
    }

    // ---------------------------------------------------------------------------
    //								    Float3x3
    // ---------------------------------------------------------------------------

    Float3x3::Float3x3()
    {
        row0 = { 1, 0, 0 };
        row1 = { 0, 1, 0 };
        row2 = { 0, 0, 1 };
    }

    Float3x3 Float3x3::operator*(const Float3x3& other)
    {
        Float3x3 out;
        out.row0 = (other.row0 * row0.x) + (other.row1 * row0.y) + (other.row2 * row0.z);
        out.row1 = (other.row0 * row1.x) + (other.row1 * row1.y) + (other.row2 * row1.z);
        out.row2 = (other.row0 * row2.x) + (other.row1 * row2.y) + (other.row2 * row2.z);
        return out;
    }

    float Float3x3::GetDeterminant() const
    {
        return   row0.x * (row1.y * row2.z - row1.z * row2.y)
               - row0.y * (row1.x * row2.z - row1.z * row2.x)
               + row0.z * (row1.x * row2.y - row1.y * row2.x);
    }
}