#pragma once

#include "Core.h"
#include "input/events/Event.h"

#include <cstdint>

namespace RB
{
	namespace Graphics
	{
		class Window;
	}

	struct AppInfo
	{
		const wchar_t* name;
		uint32_t windowWidth;
		uint32_t windowHeight;
	};

	class Application : public Input::Events::EventListener
	{
	public:
		Application(AppInfo& info);
		virtual ~Application();

		void Start(void* window_instance);
		void Run();
		void Shutdown();

		void Render();

		void OnEvent(Input::Events::Event& event) override;

		virtual void Start() = 0;
		virtual void Update() = 0;
		virtual void Stop() = 0;

		Graphics::Window* GetWindow() const { return m_Window; }

	private:
		const AppInfo		m_StartAppInfo;

		bool				m_Initialized;
		bool				m_ShouldStop;

		Graphics::Window*	m_Window;
	};
	
	// To be defined in client
	Application* CreateApplication();
}
