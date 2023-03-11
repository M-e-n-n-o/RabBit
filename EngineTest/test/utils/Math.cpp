#include "pch.h"
#include "CppUnitTest.h"

// Tested files
#include "utils/math/Vec3.h"

using namespace RB::math;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EngineTest
{
	TEST_CLASS(MathTest)
	{
	public:
		
		TEST_METHOD(Vec3AddVec3)
		{
			Vec3 first(0.5f);
			Vec3 second(0.5f);
			Vec3 result = first + second;
			Assert::AreEqual(1.0f, result.x, L"X was not correct");
			Assert::AreEqual(1.0f, result.y, L"Y was not correct");
			Assert::AreEqual(1.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3AddFloat)
		{
			Vec3 vec3(0.5f);
			Vec3 result = vec3 + 0.5f;
			Assert::AreEqual(1.0f, result.x, L"X was not correct");
			Assert::AreEqual(1.0f, result.y, L"Y was not correct");
			Assert::AreEqual(1.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3MinVec3)
		{
			Vec3 first(1.0f);
			Vec3 second(0.5f);
			Vec3 result = first - second;
			Assert::AreEqual(0.5f, result.x, L"X was not correct");
			Assert::AreEqual(0.5f, result.y, L"Y was not correct");
			Assert::AreEqual(0.5f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3MinFloat)
		{
			Vec3 vec3(1.5f);
			Vec3 result = vec3 - 0.5f;
			Assert::AreEqual(1.0f, result.x, L"X was not correct");
			Assert::AreEqual(1.0f, result.y, L"Y was not correct");
			Assert::AreEqual(1.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3MulVec3)
		{
			Vec3 first(2, 3, 4);
			Vec3 second(2, 3, 4);
			Vec3 result = first * second;
			Assert::AreEqual(4.0f, result.x, L"X was not correct");
			Assert::AreEqual(9.0f, result.y, L"Y was not correct");
			Assert::AreEqual(16.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3MulFloat)
		{
			Vec3 vec3(2, 3, 4);
			Vec3 result = vec3 * 2;
			Assert::AreEqual(4.0f, result.x, L"X was not correct");
			Assert::AreEqual(6.0f, result.y, L"Y was not correct");
			Assert::AreEqual(8.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3DivVec3)
		{
			Vec3 first(2, 4, 8);
			Vec3 second(2, 4, 8);
			Vec3 result = first / second;
			Assert::AreEqual(1.0f, result.x, L"X was not correct");
			Assert::AreEqual(1.0f, result.y, L"Y was not correct");
			Assert::AreEqual(1.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3DivFloat)
		{
			Vec3 vec3(2, 4, 8);
			Vec3 result = vec3 / 2;
			Assert::AreEqual(1.0f, result.x, L"X was not correct");
			Assert::AreEqual(2.0f, result.y, L"Y was not correct");
			Assert::AreEqual(4.0f, result.z, L"Z was not correct");
		}

		TEST_METHOD(Vec3Length)
		{
			Vec3 vec3(20, 5, 10);
			Assert::AreEqual(22.91f, vec3.GetLength(), 0.01f, L"Length was not correct");
		}

		TEST_METHOD(Vec3Normalize)
		{
			Vec3 vec3(5, 15, 10);
			vec3.Normalize();
			Assert::AreEqual(0.26f, vec3.x, 0.01f, L"X was not correct");
			Assert::AreEqual(0.80f, vec3.y, 0.01f, L"Y was not correct");
			Assert::AreEqual(0.53f, vec3.z, 0.01f, L"Z was not correct");
		}

		TEST_METHOD(Vec3Dot)
		{
			float result = Vec3::Dot(Vec3(5.2f, 3.7f, 7.9f), Vec3(2.4f, 6.1f, 9.5f));
			Assert::AreEqual(110.1f, result, 0.01f, L"Dot product was not correct");
		}

		TEST_METHOD(Vec3Angle)
		{
			Vec3 first(2, -1, 3);
			Vec3 second(2, 0, 1);
			float result = Vec3::Angle(first, second);
			Assert::AreEqual(0.58f, result, 0.01f, L"Angle was not correct");
		}

		TEST_METHOD(Vec3Cross)
		{
			Vec3 first(2, 3, 4);
			Vec3 second(5, 6, 7);
			Vec3 result = Vec3::Cross(first, second);
			Assert::AreEqual(-3.0f, result.x, 0.01f, L"X was not correct");
			Assert::AreEqual(6.0f, result.y, 0.01f, L"Y was not correct");
			Assert::AreEqual(-3.0f, result.z, 0.01f, L"Z was not correct");
		}
	};
}
