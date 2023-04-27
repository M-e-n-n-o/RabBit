#include "RabBitPch.h"
#include "Application.h"

#include "graphics/GraphicsDevice.h"

namespace RB
{
	Application::Application()
	{
		//RB_LOG_RELEASE("Welcome to the RabBit Engine");
		//RB_LOG_RELEASE("Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);

		Graphics::GraphicsDevice device;
		
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		while (true)
		{
			Update();
		}
	}
}