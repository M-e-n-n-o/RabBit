#pragma once

#include "Core.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace RB
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Start(HINSTANCE window_instance);
		void Run();
		void Shutdown();

		virtual void Start() = 0;
		virtual void Update() = 0;
		virtual void Stop() = 0;
	};
	
	// To be defined in client
	Application* CreateApplication();
}
