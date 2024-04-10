#pragma once

#include "input/events/Event.h"
#include "graphics/RenderResource.h"

namespace RB::Graphics
{
	enum WindowStyle : uint32_t
	{
		kWindowStyle_Default			= (0),
		kWindowStyle_Borderless			= (1 << 0),
		kWindowStyle_SemiTransparent	= (1 << 1)
	};

	class Window : Input::Events::EventListener
	{
	public:
		static const uint32_t BACK_BUFFER_COUNT = 2u;

		Window(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style);
		virtual ~Window();

		virtual void Update() = 0;

		virtual void Resize(uint32_t width, uint32_t height, int32_t x = -1, int32_t y = -1) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		bool IsMinimized()	const { return m_Minimized; }
		bool InFocus()		const { return m_InFocus; }
		bool ShouldClose()	const { return m_AskedForClose; }
		bool HasWindow()	const { return m_IsValid; }

		bool IsSameWindow(void* window_handle) const;

		virtual Graphics::Texture2D* GetCurrentBackBuffer() = 0;

		void OnEvent(Input::Events::Event& event);

		static Window* Create();

	private:
		void OnResize(uint32_t width, uint32_t height);
		void DestroyWindow();

		bool m_Minimized;
		bool m_IsValid;
		bool m_InFocus;
		bool m_AskedForClose;
	};
}