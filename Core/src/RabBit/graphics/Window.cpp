#include "RabBitPch.h"
#include "Window.h"
#include "graphics/native/window/NativeWindow.h"
#include "graphics/native/window/SwapChain.h"
#include "graphics/native/GraphicsDeviceEngine.h"

using namespace RB::Graphics::Native;
using namespace RB::Graphics::Native::Window;

namespace RB::Graphics
{
	Window::Window(void* window_instance, Native::GraphicsDeviceEngine* graphics_engine, Input::Events::EventListener* listener,
		const char* window_name, uint32_t window_width, uint32_t window_height)
		: m_Minimized(window_width == 0 && window_height == 0)
	{
		std::wstring w;
		std::copy(window_name, window_name + strlen(window_name), back_inserter(w));
		const WCHAR* name_wchar = w.c_str();

		g_NativeWindow = new NativeWindow(listener);
		g_NativeWindow->RegisterWindowCLass((HINSTANCE)window_instance, L"DX12WindowClass");
		g_NativeWindow->CreateWindow((HINSTANCE)window_instance, L"DX12WindowClass", name_wchar, window_width, window_height);

		g_SwapChain = new SwapChain(graphics_engine->GetCommandQueue(), window_width, window_height);

		g_NativeWindow->ShowWindow();
	}

	Window::~Window()
	{
		delete g_SwapChain;
		delete g_NativeWindow;
	}

	void Window::Update()
	{
		g_NativeWindow->ProcessEvents();
	}

	void Window::Resize(uint32_t width, uint32_t height)
	{
		if (g_SwapChain->GetWidth() == width && g_SwapChain->GetHeight() == height)
		{
			return;
		}

		m_Minimized = (width == 0 && height == 0);

		width  = std::max(1u, width);
		height = std::max(1u, height);

		g_SwapChain->Resize(width, height);
	}

	uint32_t Window::GetWidth() const
	{
		return g_SwapChain->GetWidth();
	}

	uint32_t Window::GetHeight() const
	{
		return g_SwapChain->GetHeight();
	}
}