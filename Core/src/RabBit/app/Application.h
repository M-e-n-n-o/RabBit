#pragma once

#include "Core.h"
#include "input/events/Event.h"

namespace RB
{
	class Application : public Input::Events::EventListener
	{
	public:
		Application();
		virtual ~Application();

		void Start(void* window_instance);
		void Run();
		void Shutdown();

		virtual void Start() = 0;
		virtual void Update() = 0;
		virtual void Stop() = 0;

		void OnEvent(Input::Events::Event& event) override;

	private:
		bool	m_Initialized;
		bool	m_ShouldStop;
	};
	
	// To be defined in client
	Application* CreateApplication();
}
