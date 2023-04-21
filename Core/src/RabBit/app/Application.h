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

		virtual void Update() = 0;
	};
	
	// To be defined in client
	Application* CreateApplication();
}
