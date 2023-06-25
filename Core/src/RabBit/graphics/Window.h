#pragma once

#include "input/events/Event.h"

namespace RB::Graphics
{
	class Window
	{
	public:
		static const uint32_t BACK_BUFFER_COUNT = 3u;

		Window(void* window_instance, Input::Events::EventListener* listener,
			const wchar_t* window_name, uint32_t window_width, uint32_t window_height);
		~Window();

		void Update();

		void Resize(uint32_t width, uint32_t height);

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		bool IsMinimized() const { return m_Minimized; }

	private:
		bool		m_Minimized;
	};
}