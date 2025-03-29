#pragma once

#include "Core.h"
#include "Settings.h"
#include "events/Event.h"

#include <cstdint>

namespace RB
{
	class ApplicationLayer
	{
	public:
		ApplicationLayer(const char* name) 
			: m_Name(name) 
			, m_Enabled(true)
		{}

		virtual ~ApplicationLayer() = default;

		// Gets called when attached to the layerstack
		virtual void OnAttach() {}
		// Gets called when detached from the layerstack
		virtual void OnDetach() {}
		// Gets called every game loop before the rendering
		virtual void OnUpdate() {}
		// Gets called every time an event occurs in the application
		// Returns wheter the event was processed by the layer
		virtual bool OnEvent(const Events::Event& event) { return false; }

		void SetEnabled(bool enabled) { m_Enabled = enabled; }
		bool IsEnabled() const { return m_Enabled; }
		const char* GetName() const { return m_Name; }

	private:
		const char* m_Name;
		bool		m_Enabled;
	};

	class LayerStack
	{
	public:
		LayerStack()
			: m_LayerInsertIndex(0)
		{}

		~LayerStack();

		// Add a new layer to the stack
		void PushLayer(ApplicationLayer* layer);
		// Add an overlay to the stack (gets updated before the regular layers)
		void PushOverlay(ApplicationLayer* overlay);
		// Remove a layer from the stack
		void PopLayer(ApplicationLayer* layer);
		// Remove an overlay from the stack
		void PopOverlay(ApplicationLayer* overlay);

		void ClearStack();

		List<ApplicationLayer*>::iterator begin() { return m_Layers.begin(); }
		List<ApplicationLayer*>::iterator end() { return m_Layers.end(); }

	private:
		List<ApplicationLayer*> m_Layers;
		uint32_t m_LayerInsertIndex;
	};
}
