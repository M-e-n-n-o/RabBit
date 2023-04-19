#include <gtest/gtest.h>
#include <RabBit/utils/math/Vec.h>

using namespace RB::math;
using namespace testing;

TEST(MathTest, Vec3AddVec3)
{
	Vec3 first(0.5f);
	Vec3 second(0.5f);
	Vec3 result = first + second;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(2.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Vec3AddFloat)
{
	Vec3 vec3(0.5f);
	Vec3 result = vec3 + 0.5f;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Vec3MinVec3)
{
	Vec3 first(1.0f);
	Vec3 second(0.5f);
	Vec3 result = first - second;
	ASSERT_EQ(0.5f, result.x);
	ASSERT_EQ(0.5f, result.y);
	ASSERT_EQ(0.5f, result.z);
}

TEST(MathTest, Vec3MinFloat)
{
	Vec3 vec3(1.5f);
	Vec3 result = vec3 - 0.5f;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Vec3MulVec3)
{
	Vec3 first(2, 3, 4);
	Vec3 second(2, 3, 4);
	Vec3 result = first * second;
	ASSERT_EQ(4.0f, result.x);
	ASSERT_EQ(9.0f, result.y);
	ASSERT_EQ(16.0f, result.z);
}

TEST(MathTest, Vec3MulFloat)
{
	Vec3 vec3(2, 3, 4);
	Vec3 result = vec3 * 2;
	ASSERT_EQ(4.0f, result.x);
	ASSERT_EQ(6.0f, result.y);
	ASSERT_EQ(8.0f, result.z);
}

TEST(MathTest, Vec3DivVec3)
{
	Vec3 first(2, 4, 8);
	Vec3 second(2, 4, 8);
	Vec3 result = first / second;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(1.0f, result.y);
	ASSERT_EQ(1.0f, result.z);
}

TEST(MathTest, Vec3DivFloat)
{
	Vec3 vec3(2, 4, 8);
	Vec3 result = vec3 / 2;
	ASSERT_EQ(1.0f, result.x);
	ASSERT_EQ(2.0f, result.y);
	ASSERT_EQ(4.0f, result.z);
}

TEST(MathTest, Vec3Length)
{
	Vec3 vec3(20, 5, 10);
	EXPECT_FLOAT_EQ(22.91288f, vec3.GetLength());
}

TEST(MathTest, Vec3Normalize)
{
	Vec3 vec3(5, 15, 10);
	vec3.Normalize();
	EXPECT_FLOAT_EQ(0.26726124f, vec3.x);
	EXPECT_FLOAT_EQ(0.80178374f, vec3.y);
	EXPECT_FLOAT_EQ(0.53452247f, vec3.z);
}

TEST(MathTest, Vec3Dot)
{
	float result = Vec3::Dot(Vec3(5.2f, 3.7f, 7.9f), Vec3(2.4f, 6.1f, 9.5f));
	ASSERT_EQ(110.100006f, result);
}

TEST(MathTest, Vec3Angle)
{
	Vec3 first(2, -1, 3);
	Vec3 second(2, 0, 1);
	float result = Vec3::Angle(first, second);
	ASSERT_EQ(0.579639852f, result);
}

TEST(MathTest, Vec3Cross)
{
	Vec3 first(2, 3, 4);
	Vec3 second(5, 6, 7);
	Vec3 result = Vec3::Cross(first, second);
	ASSERT_EQ(-3.0f, result.x);
	ASSERT_EQ(6.0f, result.y);
	ASSERT_EQ(-3.0f, result.z);
}
