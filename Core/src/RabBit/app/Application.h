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
		const char* name;
		uint32_t windowWidth;
		uint32_t windowHeight;
	};

	class Application : public Input::Events::EventListener
	{
	public:
		Application(AppInfo& info);
		virtual ~Application();

		void Start(const char* launch_args);
		void Run();
		void Shutdown();

		Graphics::Window* GetPrimaryWindow() const;
		Graphics::Window* GetWindow(uint32_t index) const;
		Graphics::Window* FindWindow(void* window_handle) const;

		uint64_t GetFrameIndex() const { return m_FrameIndex; }

		static Application* GetInstance() { return s_Instance; }

	private:
		void Render();

		virtual void OnStart() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnStop() = 0;

		void OnEvent(Input::Events::Event& event) override;

		const AppInfo			m_StartAppInfo;

		bool					m_Initialized;
		bool					m_ShouldStop;

		uint64_t				m_FrameIndex;

		List<Graphics::Window*>	m_Windows;
		bool					m_CheckWindows;

		static Application*		s_Instance;
	};

	// To be defined in client
	Application* CreateApplication(const char* launch_args);
}
