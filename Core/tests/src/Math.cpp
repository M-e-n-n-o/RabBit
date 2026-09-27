#include <gtest/gtest.h>
#include <rabbit/math/Misc.h>
#include <rabbit/math/Matrix.h>
#include <rabbit/math/Vector.h>
#include <rabbit/math/Quaternion.h>

using namespace RB::Math;
using namespace testing;

bool IsApproximatelyIdentity(const Float4x4& m, float epsilon = 1e-5f)
{
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			float expected = (row == col) ? 1.0f : 0.0f;
			float value = m.a[row * 4 + col];
			if (Abs(value - expected) > epsilon)
			{
				//std::cout << "Mismatch at [" << row << "][" << col << "]: " << value << " != " << expected << "\n";
				return false;
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
//											Float4x4
// ---------------------------------------------------------------------------------------------


TEST(MathTest, Float4x4Transpose)
{
	Float4x4 m;
	// Fill with known values
	m.a00 = 1;  m.a01 = 2;  m.a02 = 3;  m.a03 = 4;
	m.a10 = 5;  m.a11 = 6;  m.a12 = 7;  m.a13 = 8;
	m.a20 = 9;  m.a21 = 10; m.a22 = 11; m.a23 = 12;
	m.a30 = 13; m.a31 = 14; m.a32 = 15; m.a33 = 16;

	m.Transpose();

	// Check the result
	ASSERT_TRUE(m.a00 == 1 && m.a01 == 5 && m.a02 == 9 && m.a03 == 13);
	ASSERT_TRUE(m.a10 == 2 && m.a11 == 6 && m.a12 == 10 && m.a13 == 14);
	ASSERT_TRUE(m.a20 == 3 && m.a21 == 7 && m.a22 == 11 && m.a23 == 15);
	ASSERT_TRUE(m.a30 == 4 && m.a31 == 8 && m.a32 == 12 && m.a33 == 16);
}

TEST(MathTest, Float4x4Inverse)
{
    Float4x4 m;
    m.a[0]  = 4;  m.a[1]  = 7;  m.a[2]  = 2;  m.a[3]  = 0;
    m.a[4]  = 3;  m.a[5]  = 6;  m.a[6]  = 1;  m.a[7]  = 0;
    m.a[8]  = 2;  m.a[9]  = 5;  m.a[10] = 3;  m.a[11] = 0;
    m.a[12] = 0;  m.a[13] = 0;  m.a[14] = 0;  m.a[15] = 1;

    Float4x4 original = m;

    bool result = m.Invert();
	ASSERT_TRUE(result);

    Float4x4 identity = original * m;
	ASSERT_TRUE(IsApproximatelyIdentity(identity));
}


TEST(MathTest, Float4x4InvertAffineIdentity)
{
	Float4x4 original;

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
}

TEST(MathTest, Float4x4InvertAffineTranslation)
{
	Float4x4 original;
	original.SetPosition(10.0f, -5.0f, 3.0f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	// Inverse of pure translation should negate the translation.
	ASSERT_NEAR(inverse.a30, -10.0f, 1e-5f);
	ASSERT_NEAR(inverse.a31, 5.0f, 1e-5f);
	ASSERT_NEAR(inverse.a32, -3.0f, 1e-5f);
	ASSERT_NEAR(inverse.a33, 1.0f, 1e-5f);

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
}

TEST(MathTest, Float4x4InvertAffineRotation)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.RotateAroundZ(1.13f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
	ASSERT_TRUE(IsApproximatelyIdentity(inverse * original));
}

TEST(MathTest, Float4x4InvertAffineRotationAndTranslation)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.RotateAroundZ(1.13f);
	original.SetPosition(12.0f, -7.0f, 4.5f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
	ASSERT_TRUE(IsApproximatelyIdentity(inverse * original));
}

TEST(MathTest, Float4x4InvertAffineMatchesGeneralInverse)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.RotateAroundZ(1.13f);
	original.SetPosition(12.0f, -7.0f, 4.5f);

	Float4x4 affineInverse = original;
	affineInverse.InvertAffine();

	Float4x4 generalInverse = original;
	ASSERT_TRUE(generalInverse.Invert());

	// Both inverse implementations should produce the same result.
	for (int i = 0; i < 16; ++i)
	{
		ASSERT_NEAR(
			affineInverse.a[i],
			generalInverse.a[i],
			1e-5f);
	}
}

TEST(MathTest, Float4x4InvertAffineNonUniformScale)
{
	Float4x4 original;
	original.Scale(2.0f, 3.0f, 4.0f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_NEAR(inverse.a00, 0.5f, 1e-5f);
	ASSERT_NEAR(inverse.a11, 1.0f / 3.0f, 1e-5f);
	ASSERT_NEAR(inverse.a22, 0.25f, 1e-5f);

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
	ASSERT_TRUE(IsApproximatelyIdentity(inverse * original));
}

TEST(MathTest, Float4x4InvertAffineNonUniformScaleAndTranslation)
{
	Float4x4 original;
	original.Scale(2.0f, 3.0f, 4.0f);
	original.SetPosition(10.0f, -5.0f, 3.0f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_NEAR(inverse.a00, 0.5f, 1e-5f);
	ASSERT_NEAR(inverse.a11, 1.0f / 3.0f, 1e-5f);
	ASSERT_NEAR(inverse.a22, 0.25f, 1e-5f);

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
	ASSERT_TRUE(IsApproximatelyIdentity(inverse * original));
}

TEST(MathTest, Float4x4InvertAffineRotationAndNonUniformScale)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.RotateAroundZ(1.13f);
	original.Scale(2.0f, 3.0f, 4.0f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
	ASSERT_TRUE(IsApproximatelyIdentity(inverse * original));
}

TEST(MathTest, Float4x4InvertAffineRotationNonUniformScaleAndTranslation)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.RotateAroundZ(1.13f);
	original.Scale(2.0f, 3.0f, 4.0f);
	original.SetPosition(12.0f, -7.0f, 4.5f);

	Float4x4 inverse = original;
	inverse.InvertAffine();

	ASSERT_TRUE(IsApproximatelyIdentity(original * inverse));
	ASSERT_TRUE(IsApproximatelyIdentity(inverse * original));
}

TEST(MathTest, Float4x4InvertAffineNonUniformScaleMatchesGeneralInverse)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.RotateAroundZ(1.13f);
	original.Scale(2.0f, 3.0f, 4.0f);
	original.SetPosition(12.0f, -7.0f, 4.5f);

	Float4x4 affineInverse = original;
	affineInverse.InvertAffine();

	Float4x4 generalInverse = original;
	ASSERT_TRUE(generalInverse.Invert());

	for (int i = 0; i < 16; ++i)
	{
		ASSERT_NEAR(
			affineInverse.a[i],
			generalInverse.a[i],
			1e-5f);
	}
}

TEST(MathTest, Float4x4InvertAffineDifferentNonUniformScales)
{
	Float4x4 original;
	original.RotateAroundX(-0.43f);
	original.RotateAroundY(0.71f);
	original.RotateAroundZ(-1.27f);
	original.Scale(0.25f, 7.0f, 1.75f);
	original.SetPosition(-13.0f, 2.5f, 8.0f);

	Float4x4 affineInverse = original;
	affineInverse.InvertAffine();

	Float4x4 generalInverse = original;
	ASSERT_TRUE(generalInverse.Invert());

	ASSERT_TRUE(IsApproximatelyIdentity(original * affineInverse));
	ASSERT_TRUE(IsApproximatelyIdentity(affineInverse * original));

	for (int i = 0; i < 16; ++i)
	{
		ASSERT_NEAR(
			affineInverse.a[i],
			generalInverse.a[i],
			1e-5f);
	}
}

TEST(MathTest, Float4x4InvertAffineNegativeNonUniformScale)
{
	Float4x4 original;
	original.RotateAroundX(0.37f);
	original.RotateAroundY(-0.82f);
	original.Scale(-2.0f, 3.0f, -4.0f);
	original.SetPosition(5.0f, -6.0f, 7.0f);

	Float4x4 affineInverse = original;
	affineInverse.InvertAffine();

	Float4x4 generalInverse = original;
	ASSERT_TRUE(generalInverse.Invert());

	ASSERT_TRUE(IsApproximatelyIdentity(original * affineInverse));
	ASSERT_TRUE(IsApproximatelyIdentity(affineInverse * original));

	for (int i = 0; i < 16; ++i)
	{
		ASSERT_NEAR(
			affineInverse.a[i],
			generalInverse.a[i],
			1e-5f);
	}
}

TEST(MathTest, Float4x4InvertAffineUniformScale)
{
	Float4x4 original;
	original.RotateAroundX(0.51f);
	original.RotateAroundY(-0.33f);
	original.RotateAroundZ(1.17f);
	original.Scale(3.5f);
	original.SetPosition(4.0f, -8.0f, 2.0f);

	Float4x4 affineInverse = original;
	affineInverse.InvertAffine();

	Float4x4 generalInverse = original;
	ASSERT_TRUE(generalInverse.Invert());

	ASSERT_TRUE(IsApproximatelyIdentity(original * affineInverse));
	ASSERT_TRUE(IsApproximatelyIdentity(affineInverse * original));

	for (int i = 0; i < 16; ++i)
	{
		ASSERT_NEAR(
			affineInverse.a[i],
			generalInverse.a[i],
			1e-5f);
	}
}

// ---------------------------------------------------------------------------------------------
//											Float3
// ---------------------------------------------------------------------------------------------

TEST(MathTest, Float3AddFloat3)
{
	Float3 first(0.5f);
	Float3 second(0.5f);
	Float3 result = first + second;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Float3AddFloat)
{
	Float3 float3(0.5f);
	Float3 result = float3 + 0.5f;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Float3MinFloat3)
{
	Float3 first(1.0f);
	Float3 second(0.5f);
	Float3 result = first - second;
	ASSERT_EQ(0.5f, result.x);
	ASSERT_EQ(0.5f, result.y);
	ASSERT_EQ(0.5f, result.z);
}

TEST(MathTest, Float3MinFloat)
{
	Float3 float3(1.5f);
	Float3 result = float3 - 0.5f;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Float3MulFloat3)
{
	Float3 first(2, 3, 4);
	Float3 second(2, 3, 4);
	Float3 result = first * second;
	ASSERT_EQ(4.0f, result.x);
	ASSERT_EQ(9.0f, result.y);
	ASSERT_EQ(16.0f, result.z);
}

TEST(MathTest, Float3MulFloat)
{
	Float3 float3(2, 3, 4);
	Float3 result = float3 * 2;
	ASSERT_EQ(4.0f, result.x);
	ASSERT_EQ(6.0f, result.y);
	ASSERT_EQ(8.0f, result.z);
}

TEST(MathTest, Float3DivFloat3)
{
	Float3 first(2, 4, 8);
	Float3 second(2, 4, 8);
	Float3 result = first / second;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Float3DivFloat)
{
	Float3 float3(2, 4, 8);
	Float3 result = float3 / 2;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(2.0f, result.y);
	ASSERT_EQ(4.0f, result.z);
}

TEST(MathTest, Float3Length)
{
	Float3 float3(20, 5, 10);
	EXPECT_FLOAT_EQ(22.91288f, float3.GetLength());
}

TEST(MathTest, Float3Normalize)
{
	Float3 float3(5, 15, 10);
	float3.Normalize();
	EXPECT_FLOAT_EQ(0.26726124f, float3.x);
	EXPECT_FLOAT_EQ(0.80178374f, float3.y);
	EXPECT_FLOAT_EQ(0.53452247f, float3.z);
}

TEST(MathTest, Float3Dot)
{
	float result = Float3::Dot(Float3(5.2f, 3.7f, 7.9f), Float3(2.4f, 6.1f, 9.5f));
	ASSERT_EQ(110.100006f, result);
}

TEST(MathTest, Float3Angle)
{
	Float3 first(2, -1, 3);
	Float3 second(2, 0, 1);
	float result = Float3::Angle(first, second);
	ASSERT_EQ(0.579639852f, result);
}

TEST(MathTest, Float3Cross)
{
	Float3 first(2, 3, 4);
	Float3 second(5, 6, 7);
	Float3 result = Float3::Cross(first, second);
	ASSERT_EQ(-3.0f, result.x);
	ASSERT_EQ(6.0f, result.y);
	ASSERT_EQ(-3.0f, result.z);
}

// ---------------------------------------------------------------------------------------------
//											Quaternion
// ---------------------------------------------------------------------------------------------

TEST(MathTest, QuaternionIdentity)
{
    Quaternion q;

    ASSERT_NEAR(q.x, 0.0f, 1e-5f);
    ASSERT_NEAR(q.y, 0.0f, 1e-5f);
    ASSERT_NEAR(q.z, 0.0f, 1e-5f);
    ASSERT_NEAR(q.w, 1.0f, 1e-5f);

    ASSERT_NEAR(q.GetLength(), 1.0f, 1e-5f);
}

TEST(MathTest, QuaternionIdentityStatic)
{
    Quaternion q = Quaternion::Identity();

    ASSERT_NEAR(q.x, 0.0f, 1e-5f);
    ASSERT_NEAR(q.y, 0.0f, 1e-5f);
    ASSERT_NEAR(q.z, 0.0f, 1e-5f);
    ASSERT_NEAR(q.w, 1.0f, 1e-5f);
}

TEST(MathTest, QuaternionNormalize)
{
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);

    float originalLength = q.GetLength();
    ASSERT_GT(originalLength, 0.0f);

    q.Normalize();

    ASSERT_NEAR(q.GetLength(), 1.0f, 1e-5f);
}

TEST(MathTest, QuaternionConjugate)
{
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);

    Quaternion conjugate = q.GetConjugate();

    ASSERT_NEAR(conjugate.x, -1.0f, 1e-5f);
    ASSERT_NEAR(conjugate.y, -2.0f, 1e-5f);
    ASSERT_NEAR(conjugate.z, -3.0f, 1e-5f);
    ASSERT_NEAR(conjugate.w, 4.0f, 1e-5f);
}

TEST(MathTest, QuaternionInverse)
{
    Quaternion q = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        0.7f);

    Quaternion inverse = q.GetInverse();
    Quaternion result = q * inverse;

    ASSERT_NEAR(result.x, 0.0f, 1e-5f);
    ASSERT_NEAR(result.y, 0.0f, 1e-5f);
    ASSERT_NEAR(result.z, 0.0f, 1e-5f);
    ASSERT_NEAR(result.w, 1.0f, 1e-5f);
}

TEST(MathTest, QuaternionAxisAngle)
{
    Quaternion q = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        0.5f);

    ASSERT_NEAR(q.GetLength(), 1.0f, 1e-5f);

    // For Y axis:
    // q = (0, sin(angle/2), 0, cos(angle/2))
    ASSERT_NEAR(q.x, 0.0f, 1e-5f);
    ASSERT_NEAR(q.y, Sin(0.25f), 1e-5f);
    ASSERT_NEAR(q.z, 0.0f, 1e-5f);
    ASSERT_NEAR(q.w, Cos(0.25f), 1e-5f);
}

TEST(MathTest, QuaternionRotateVector)
{
    Quaternion q = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        DegreesToRadians(90.0f));

    Float3 v(1.0f, 0.0f, 0.0f);
    Float3 result = q.Rotate(v);

    // Matches Float4x4::RotateAroundY convention.
    ASSERT_NEAR(result.x, 0.0f, 1e-5f);
    ASSERT_NEAR(result.y, 0.0f, 1e-5f);
    ASSERT_NEAR(result.z, -1.0f, 1e-5f);
}

TEST(MathTest, QuaternionToMatrix)
{
    Quaternion q = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        0.5f);

    Float4x4 quaternionMatrix = q.ToMatrix();

    Float4x4 expected;
    expected.RotateAroundY(0.5f);

    for (int i = 0; i < 16; ++i)
    {
        ASSERT_NEAR(
            quaternionMatrix.a[i],
            expected.a[i],
            1e-5f);
    }
}

TEST(MathTest, QuaternionFromMatrix)
{
    Float4x4 matrix;
    matrix.RotateAroundX(0.3f);
    matrix.RotateAroundY(-0.7f);
    matrix.RotateAroundZ(1.1f);

    Quaternion q = Quaternion::FromMatrix(matrix);

    Float4x4 result = q.ToMatrix();

    for (int i = 0; i < 16; ++i)
    {
        ASSERT_NEAR(
            result.a[i],
            matrix.a[i],
            1e-5f);
    }
}

TEST(MathTest, QuaternionMultiplicationMatchesMatrixComposition)
{
    Quaternion a = Quaternion::FromAxisAngle(
        Float3(1.0f, 0.0f, 0.0f),
        0.4f);

    Quaternion b = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        -0.8f);

    Quaternion combined = a * b;

    Float4x4 quaternionMatrix = combined.ToMatrix();
    Float4x4 matrixComposition = a.ToMatrix() * b.ToMatrix();

    for (int i = 0; i < 16; ++i)
    {
        ASSERT_NEAR(
            quaternionMatrix.a[i],
            matrixComposition.a[i],
            1e-5f);
    }
}

TEST(MathTest, QuaternionMultiplicationMatchesVectorComposition)
{
    Quaternion a = Quaternion::FromAxisAngle(
        Float3(1.0f, 0.0f, 0.0f),
        0.4f);

    Quaternion b = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        -0.8f);

    Quaternion combined = a * b;

    Float3 v(1.0f, 2.0f, 3.0f);

    Float3 expected = b.Rotate(a.Rotate(v));
    Float3 result = combined.Rotate(v);

    ASSERT_NEAR(result.x, expected.x, 1e-5f);
    ASSERT_NEAR(result.y, expected.y, 1e-5f);
    ASSERT_NEAR(result.z, expected.z, 1e-5f);
}

TEST(MathTest, QuaternionFromEuler)
{
    const float x = 0.3f;
    const float y = -0.7f;
    const float z = 1.1f;

    Quaternion q = Quaternion::FromEuler(x, y, z);

    Float4x4 quaternionMatrix = q.ToMatrix();

    Float4x4 expected;
    expected.RotateAroundX(x);
    expected.RotateAroundY(y);
    expected.RotateAroundZ(z);

    for (int i = 0; i < 16; ++i)
    {
        ASSERT_NEAR(
            quaternionMatrix.a[i],
            expected.a[i],
            1e-5f);
    }
}

TEST(MathTest, QuaternionToEuler)
{
    const float x = 0.3f;
    const float y = -0.5f;
    const float z = 0.8f;

    Quaternion q = Quaternion::FromEuler(x, y, z);
    Float3 euler = q.ToEuler();

    ASSERT_NEAR(euler.x, x, 1e-5f);
    ASSERT_NEAR(euler.y, y, 1e-5f);
    ASSERT_NEAR(euler.z, z, 1e-5f);
}

TEST(MathTest, QuaternionLerpEndpoints)
{
    Quaternion a = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        0.2f);

    Quaternion b = Quaternion::FromAxisAngle(
        Float3(1.0f, 0.0f, 0.0f),
        0.8f);

    Quaternion start = Quaternion::Lerp(a, b, 0.0f);
    Quaternion end = Quaternion::Lerp(a, b, 1.0f);

    ASSERT_NEAR(start.x, a.x, 1e-5f);
    ASSERT_NEAR(start.y, a.y, 1e-5f);
    ASSERT_NEAR(start.z, a.z, 1e-5f);
    ASSERT_NEAR(start.w, a.w, 1e-5f);

    ASSERT_NEAR(end.x, b.x, 1e-5f);
    ASSERT_NEAR(end.y, b.y, 1e-5f);
    ASSERT_NEAR(end.z, b.z, 1e-5f);
    ASSERT_NEAR(end.w, b.w, 1e-5f);
}

TEST(MathTest, QuaternionSlerpEndpoints)
{
    Quaternion a = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        0.2f);

    Quaternion b = Quaternion::FromAxisAngle(
        Float3(1.0f, 0.0f, 0.0f),
        0.8f);

    Quaternion start = Quaternion::Slerp(a, b, 0.0f);
    Quaternion end = Quaternion::Slerp(a, b, 1.0f);

    ASSERT_NEAR(start.x, a.x, 1e-5f);
    ASSERT_NEAR(start.y, a.y, 1e-5f);
    ASSERT_NEAR(start.z, a.z, 1e-5f);
    ASSERT_NEAR(start.w, a.w, 1e-5f);

    ASSERT_NEAR(end.x, b.x, 1e-5f);
    ASSERT_NEAR(end.y, b.y, 1e-5f);
    ASSERT_NEAR(end.z, b.z, 1e-5f);
    ASSERT_NEAR(end.w, b.w, 1e-5f);
}

TEST(MathTest, QuaternionSlerpHalfway)
{
    Quaternion identity = Quaternion::Identity();

    Quaternion target = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        1.0f);

    Quaternion halfway = Quaternion::Slerp(identity, target, 0.5f);

    Float3 v(1.0f, 0.0f, 0.0f);

    Float3 result = halfway.Rotate(v);
    Float3 expected = target.Rotate(
        identity.Rotate(v)); // Only establishes the target direction.

    Quaternion expectedRotation = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        0.5f);

    Float3 expectedVector = expectedRotation.Rotate(v);

    ASSERT_NEAR(result.x, expectedVector.x, 1e-5f);
    ASSERT_NEAR(result.y, expectedVector.y, 1e-5f);
    ASSERT_NEAR(result.z, expectedVector.z, 1e-5f);
}

TEST(MathTest, QuaternionLookRotation)
{
    Float3 forward(0.0f, 0.0f, 1.0f);
    Float3 up(0.0f, 1.0f, 0.0f);

    Quaternion q = Quaternion::LookRotation(forward, up);

    Float3 result = q.Rotate(WorldForward);

    ASSERT_NEAR(result.x, forward.x, 1e-5f);
    ASSERT_NEAR(result.y, forward.y, 1e-5f);
    ASSERT_NEAR(result.z, forward.z, 1e-5f);
}

TEST(MathTest, QuaternionLookRotationRotatedForward)
{
    Float3 forward(1.0f, 0.0f, 0.0f);
    Float3 up(0.0f, 1.0f, 0.0f);

    Quaternion q = Quaternion::LookRotation(forward, up);

    Float3 result = q.Rotate(WorldForward);

    ASSERT_NEAR(result.x, forward.x, 1e-5f);
    ASSERT_NEAR(result.y, forward.y, 1e-5f);
    ASSERT_NEAR(result.z, forward.z, 1e-5f);
}

TEST(MathTest, QuaternionRotateMatchesMatrix)
{
    Quaternion q = Quaternion::FromAxisAngle(
        Float3(0.0f, 1.0f, 0.0f),
        1.2f);

    Float3 v(1.0f, 2.0f, 3.0f);

    Float3 quaternionResult = q.Rotate(v);

    Float4x4 matrix = q.ToMatrix();

    // Row-vector multiplication: v * matrix
    Float3 matrixResult(
        v.x * matrix.a00 + v.y * matrix.a10 + v.z * matrix.a20,
        v.x * matrix.a01 + v.y * matrix.a11 + v.z * matrix.a21,
        v.x * matrix.a02 + v.y * matrix.a12 + v.z * matrix.a22
    );

    ASSERT_NEAR(quaternionResult.x, matrixResult.x, 1e-5f);
    ASSERT_NEAR(quaternionResult.y, matrixResult.y, 1e-5f);
    ASSERT_NEAR(quaternionResult.z, matrixResult.z, 1e-5f);
}