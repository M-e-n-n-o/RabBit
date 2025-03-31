#include "RabBitCommon.h"
#include "ApplicationLayer.h"

namespace RB
{
	LayerStack::~LayerStack()
	{
	}

	void LayerStack::PushLayer(ApplicationLayer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
	}

	void LayerStack::PushOverlay(ApplicationLayer* overlay)
	{
		m_Layers.emplace(m_Layers.begin(), overlay);
		m_LayerInsertIndex++;
	}

	bool LayerStack::PopLayer(ApplicationLayer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			// Overlay
			m_Layers.erase(it);
			m_LayerInsertIndex--;
			return true;
		}

		it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			// Regular layer
			m_Layers.erase(it);
			return true;
		}

		return false;
	}

	void LayerStack::ClearStack()
	{
		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}
}
