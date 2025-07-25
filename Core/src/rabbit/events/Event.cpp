#include "RabBitCommon.h"
#include "Event.h"

namespace RB::Events
{
    // ----------------------------------------------------------------------------
    //									Event
    // ----------------------------------------------------------------------------

    Event::Event()
    {
    }

    Event::~Event()
    {
    }

    bool Event::IsInCategory(const EventCategory cat) const
    {
        return (GetCategoryFlags() & cat) > 0;
    }

    // ----------------------------------------------------------------------------
    //								EventManager
    // ----------------------------------------------------------------------------

    EventManager* g_EventManager = nullptr;

    EventManager::EventManager()
    {

    }

    EventManager::~EventManager()
    {
        //for (int i = 0; i < EVENT_HISTORY_COUNT; i++)
        //{
        //	delete m_LastEvents[i];
        //}

        //m_LastEvents.clear();
    }

    void EventManager::AddListener(EventListener* listener)
    {
        m_Listeners.push_back(listener);
    }

    void EventManager::RemoveListener(EventListener* listener)
    {
        m_Listeners.erase(std::remove(m_Listeners.begin(), m_Listeners.end(), listener), m_Listeners.end());
    }

    void EventManager::InsertEvent(const Event& event)
    {
        //m_LastEvents.insert(m_LastEvents.begin(), event);

        for (int i = 0; i < m_Listeners.size(); ++i)
        {
            if (m_Listeners[i]->ListensToCategory((EventCategory)event.GetCategoryFlags()))
            {
                m_Listeners[i]->AddEvent(event);
            }
        }

        //while (m_LastEvents.size() > EVENT_HISTORY_COUNT)
        //{
        //	delete m_LastEvents[m_LastEvents.size() - 1];
        //	m_LastEvents.pop_back();
        //}
    }

    // ----------------------------------------------------------------------------
    //								EventListener
    // ----------------------------------------------------------------------------

    EventListener::EventListener(int category, bool double_queue)
        : m_ListenerCategory(category)
        , m_DoubleQueue(double_queue)
    {
        m_QueuedEvents0.reserve(10);
        if (m_DoubleQueue)
            m_QueuedEvents1.reserve(10);

        g_EventManager->AddListener(this);
    }

    EventListener::~EventListener()
    {
        g_EventManager->RemoveListener(this);
    }

    void EventListener::ProcessEvents()
    {
        auto process_events = [this](List<Event*>& queue)
        {
            for (auto itr = queue.begin(); itr < queue.end();)
            {
                Event* e = *itr;

                bool handled = OnEvent(*e);

                if (handled)
                {
                    itr = queue.erase(itr);
                    delete e;
                }
                else
                {
                    // Do it next time
                    itr++;
                }
            }
        };

        if (m_DoubleQueue)
        {
            // TODO This goes wrong when you do alt-enter, then you try to lock the mutex on the same thread which already has locked the mutex, we need to somehow avoid this!
            static_assert(false);

            m_Mutex.lock();
            m_QueueCycle = !m_QueueCycle;
            List<Event*>& queue = m_QueueCycle ? m_QueuedEvents0 : m_QueuedEvents1;
            m_Mutex.unlock();

            process_events(queue);
        }
        else
        {
            m_Mutex.lock();
            process_events(m_QueuedEvents0);
            m_Mutex.unlock();
        }
    }

    void EventListener::AddEvent(const Event& e)
    {
        auto add_event = [&e](List<Event*>& queue)
        {
            if (e.AllowOverwrite())
            {
                auto itr = std::find_if(queue.begin(), queue.end(), [&e](Event* other) -> bool {
                    return e.GetEventType() == other->GetEventType() && other->IsOverwritable(&e);
                });

                if (itr != queue.end())
                {
                    queue.erase(itr);
                }
            }

            queue.push_back(e.Clone());
        };

        if (m_DoubleQueue)
        {
            m_Mutex.lock();
            List<Event*>& queue = m_QueueCycle ? m_QueuedEvents1 : m_QueuedEvents0;
            m_Mutex.unlock();

            add_event(queue);
        }
        else
        {
            m_Mutex.lock();
            add_event(m_QueuedEvents0);
            m_Mutex.unlock();
        }
    }
}