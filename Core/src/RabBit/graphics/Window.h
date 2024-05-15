#pragma once

#include "input/events/WindowEvent.h"
#include "graphics/RenderResource.h"

namespace RB::Graphics
{
	enum WindowStyle : uint32_t
	{
		kWindowStyle_Default			= (0),
		kWindowStyle_Borderless			= (1 << 0),
		kWindowStyle_SemiTransparent	= (1 << 1)
	};

	enum class VsyncMode : uint8_t
	{
		Off		= 0,
		On		= 1,
		Half	= 2,
		Quarter	= 3,
		Eighth	= 4
	};

	class Window
	{
	public:
		static const uint32_t BACK_BUFFER_COUNT = 2u;

		virtual ~Window();

		virtual void Update() = 0;

		virtual void Present(const VsyncMode& mode) = 0;

		virtual void Resize(uint32_t width, uint32_t height, int32_t x = -1, int32_t y = -1) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual bool IsMinimized()	const = 0;
		virtual bool IsValid()		const = 0;

		bool InFocus()		const;

		virtual bool IsSameWindow(void* window_handle) const = 0;

		virtual RenderResourceFormat GetBackBufferFormat() = 0;
		virtual uint32_t GetCurrentBackBufferIndex() = 0;
		virtual Graphics::Texture2D* GetCurrentBackBuffer() = 0;

		void ProcessEvent(Input::Events::WindowEvent& event);

		static Window* Create(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style);
		
	protected:
		Window();

		virtual void DestroyWindow() = 0;
	private:
		virtual void OnResize(uint32_t width, uint32_t height) = 0;

		bool m_InFocus;
	};
}