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

		void Start();
		void Run();
		void Shutdown();

		void StartRenderFrame();
		void Render();
		void FinishRenderFrame();

		void OnEvent(Input::Events::Event& event) override;

		virtual void OnStart() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnStop() = 0;

		Graphics::Window* GetWindow() const { return m_Window; }

		uint64_t GetFrameIndex() const { return m_FrameIndex; }

	private:
		const AppInfo		m_StartAppInfo;

		bool				m_Initialized;
		bool				m_ShouldStop;

		uint64_t			m_FrameIndex;

		Graphics::Window*	m_Window;
	};
	
	// To be defined in client
	Application* CreateApplication();
}
