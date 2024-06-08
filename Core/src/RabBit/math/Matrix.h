#pragma once

#include "Core.h"

namespace RB::Math
{
	struct Float4x4
	{
	public:
		union
		{
			struct
			{
				float a[16];
			};

			struct
			{
				Float4 row0;
				Float4 row1;
				Float4 row2;
				Float4 row3;
			};

			struct
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

		Float3 GetPosition();

		void SetPosition(const Float3 pos);
		void SetPosition(float x, float y, float z);

		void Scale(const Float3 scale);
		void Scale(float scale);
		void Scale(float x, float y, float z);
	};
}