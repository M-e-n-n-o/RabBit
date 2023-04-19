#pragma once

#include "Core.h"

namespace RB
{
	class Application
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
