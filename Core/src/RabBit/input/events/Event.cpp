#include "RabBitCommon.h"
#include "Event.h"

namespace RB::Input::Events
{
	EventManager* g_EventManager = nullptr;

	EventManager::EventManager()
	{
		
	}

	EventManager::~EventManager()
	{
		for (int i = 0; i < EVENT_HISTORY_COUNT; i++)
		{
			delete m_LastEvents[i];
		}

		m_LastEvents.clear();
	}

	void EventManager::AddListener(EventListener* listener)
	{
		m_Listeners.push_back(listener);
	}

	void EventManager::RemoveListener(EventListener* listener)
	{
		m_Listeners.erase(std::remove(m_Listeners.begin(), m_Listeners.end(), listener), m_Listeners.end());
	}

	void EventManager::InsertEvent(Event* event) 
	{
		m_LastEvents.insert(m_LastEvents.begin(), event);

		for (EventListener* listener : m_Listeners)
		{
			if (listener->ListensToCategory((EventCategory) event->GetCategoryFlags()))
			{
				listener->OnEvent(*event);
			}
		}

		if (m_LastEvents.size() > EVENT_HISTORY_COUNT)
		{
			delete m_LastEvents[EVENT_HISTORY_COUNT];
			m_LastEvents.pop_back();
		}
	}
}