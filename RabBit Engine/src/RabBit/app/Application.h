#pragma once

#include "Core.h"

namespace RabBit
{
	class RABBIT_API Application
	{
	public:
		Application();
		virtual ~Application();

		virtual int TestValue() = 0;
	};

	// To be defined in client
	Application* CreateApplication();
}
