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
        Float4x4 copy = *this;

        float sin_theta = Sin(xrad);
        float cos_theta = Cos(xrad);

        for (int i = 0; i < 4; ++i)
        {
            copy.row[i].y = cos_theta * row[i].y - sin_theta * row[i].z;
            copy.row[i].z = sin_theta * row[i].y + cos_theta * row[i].z;
            copy.row[i].x = row[i].x;
            copy.row[i].w = row[i].w;
        }

        memcpy(a, copy.a, 16 * sizeof(float));
    }

    void Float4x4::RotateAroundY(float yrad)
    {
        Float4x4 copy = *this;

        float sin_theta = Sin(yrad);
        float cos_theta = Cos(yrad);

        for (int i = 0; i < 4; ++i)
        {
            copy.row[i].z = cos_theta * row[i].z - sin_theta * row[i].x;
            copy.row[i].x = sin_theta * row[i].z + cos_theta * row[i].x;
            copy.row[i].y = row[i].y;
            copy.row[i].w = row[i].w;
        }

        memcpy(a, copy.a, 16 * sizeof(float));
    }

    void Float4x4::RotateAroundZ(float zrad)
    {
        Float4x4 copy = *this;

        float sin_theta = Sin(zrad);
        float cos_theta = Cos(zrad);

        for (int i = 0; i < 4; ++i)
        {
            copy.row[i].x = cos_theta * row[i].x - sin_theta * row[i].y;
            copy.row[i].y = sin_theta * row[i].x + cos_theta * row[i].y;
            copy.row[i].z = row[i].z;
            copy.row[i].w = row[i].w;
        }

        memcpy(a, copy.a, 16 * sizeof(float));
    }

    Float3 Float4x4::GetPosition()
    {
        return Float3(a30, a31, a32);
    }

    void Float4x4::SetPosition(const Float3 pos)
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

    void Float4x4::Scale(const Float3 scale)
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