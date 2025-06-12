#include <gtest/gtest.h>
#include <RabBit/math/Matrix.h>
#include <RabBit/math/Vector.h>

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
			if (std::fabs(value - expected) > epsilon)
			{
				//std::cout << "Mismatch at [" << row << "][" << col << "]: " << value << " != " << expected << "\n";
				return false;
			}
		}
	}
	return true;
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
    assert(result && "Matrix should be invertible");

    Float4x4 identity = original * m;
	ASSERT_TRUE(IsApproximatelyIdentity(identity));
}

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
