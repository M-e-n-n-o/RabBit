#pragma once

#include "RabBitCommon.h"
#include "graphics/Window.h"
#include "SwapChain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace RB::Graphics::D3D12
{
	struct WindowArgs
	{
		HINSTANCE		instance;
		wchar_t*		className;
		wchar_t*		windowName;
		uint32_t		width;
		uint32_t		height;
		DWORD			extendedStyle;
		DWORD			style;
		bool			borderless;
	};

	class WindowD3D12 : public Window
	{
	public:
		WindowD3D12(const WindowArgs args);
		~WindowD3D12();

		void Update() override;

		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;

		void Resize(uint32_t width, uint32_t height, int32_t x, int32_t y) override;
		Graphics::Texture2D* GetCurrentBackBuffer() override;

		void ShowWindow();

		HWND GetHandle() const { return m_WindowHandle; }

	private:
		void OnResize(uint32_t width, uint32_t height) override;
		void DestroyWindow() override;

		void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name);

		void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style, bool borderless);

		HWND		m_WindowHandle;
		SwapChain*	m_SwapChain;
	};
}