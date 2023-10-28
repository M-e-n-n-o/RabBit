#pragma once

#include "input/events/Event.h"

namespace RB::Graphics
{
	namespace Native::Window
	{
		class NativeWindow;
		class SwapChain;
	}

	enum WindowStyle : uint32_t
	{
		kWindowStyle_None				= (0),
		kWindowStyle_Borderless			= (1 << 0),
		kWindowStyle_SemiTransparent	= (1 << 1)
	};

	class Window : Input::Events::EventListener
	{
	public:
		static const uint32_t BACK_BUFFER_COUNT = 2u;

		Window(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style);
		virtual ~Window();

		void Update();

		void Resize(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		bool IsMinimized() const { return m_Minimized; }

		bool IsValid() const { return m_IsValid; }

		Native::Window::SwapChain* GetSwapChain() const { return m_SwapChain; }

		void OnEvent(Input::Events::Event& event);
	private:
		void OnResize(uint32_t width, uint32_t height);
		void DestroyWindow();

		Native::Window::NativeWindow*	m_NativeWindow;
		Native::Window::SwapChain*		m_SwapChain;
		bool							m_Minimized;
		bool							m_IsValid;
	};
}