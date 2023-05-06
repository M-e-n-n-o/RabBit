#include "RabBitPch.h"
#include "Application.h"

#include "graphics/NativeWindow.h"
#include "graphics/GraphicsDevice.h"

using namespace RB::Graphics;

namespace RB
{
	Application::Application()
	{
		RB_LOG_RELEASE("Welcome to the RabBit Engine");
		RB_LOG_RELEASE("Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);	
	}

	Application::~Application()
	{

	}

	void Application::Start(HINSTANCE window_instance)
	{
		g_NativeWindow = new NativeWindow();
		g_NativeWindow->RegisterWindowCLass(window_instance, L"DX12WindowClass");
		g_NativeWindow->CreateWindow(window_instance, L"DX12WindowClass", L"RabBit App", 1920, 1080);

		g_GraphicsDevice = new GraphicsDevice();

		g_NativeWindow->ShowWindow();
	}

	void Application::Run()
	{
		while (true)
		{
			Update();
		}
	}
}