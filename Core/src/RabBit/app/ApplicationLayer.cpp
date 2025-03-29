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
		m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(ApplicationLayer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(ApplicationLayer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}
	
	void LayerStack::PopOverlay(ApplicationLayer* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
		}
	}

	void LayerStack::ClearStack()
	{
		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}
}
