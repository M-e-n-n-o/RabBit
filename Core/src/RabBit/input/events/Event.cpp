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
		// TODO Do not directly execute the events as some will have to be done on a separate thread (window resizing on render thread).
		// So just put the events in a queue that will be executed in order when the necessary thread is ready to execute them.

		m_LastEvents.insert(m_LastEvents.begin(), event);

		for (EventListener* listener : m_Listeners)
		{
			if (listener->ListensToCategory((EventCategory) event->GetCategoryFlags()))
			{
				listener->OnEvent(*event);
			}
		}

		while (m_LastEvents.size() > EVENT_HISTORY_COUNT)
		{
			delete m_LastEvents[m_LastEvents.size() - 1];
			m_LastEvents.pop_back();
		}
	}
}