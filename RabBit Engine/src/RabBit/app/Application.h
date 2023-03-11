#pragma once

#include "Core.h"

namespace RB
{
	class RABBIT_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		virtual int TestValue() = 0;
		virtual void Update() = 0;
	};

	// To be defined in client
	Application* CreateApplication();
}
