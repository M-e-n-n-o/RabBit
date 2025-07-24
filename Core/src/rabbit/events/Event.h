#pragma once

#include "RabBitCommon.h"

namespace RB::Events
{
    enum class EventType
    {
        None = 0,
        WindowCreated, WindowCloseRequest, WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowFullscreenToggle,
        KeyPressed, KeyReleased, KeyTyped,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
        GraphicsSettingsChanged
    };

    enum EventCategory
    {
        kEventCat_None          = 0,
        kEventCat_Window        = (1 << 0),
        kEventCat_Keyboard      = (1 << 1),
        kEventCat_Mouse         = (1 << 2),
        kEventCat_MouseButton   = (1 << 3),
        kEventCat_Application   = (1 << 4),

        kEventCat_Input = (kEventCat_Keyboard | kEventCat_Mouse | kEventCat_MouseButton),
        kEventCat_All = (kEventCat_Window | kEventCat_Input | kEventCat_Keyboard | kEventCat_Mouse | kEventCat_MouseButton | kEventCat_Application)
    };

    #define DEFINE_CLASS_TYPE_CUSTOM_OVERWRITE(classType, type) static EventType GetStaticType() { return EventType::type; }\
								                                virtual EventType GetEventType() const override { return GetStaticType(); }\
								                                virtual const char* GetName() const override { return #type; }\
								                                virtual Event* Clone() const override { return new classType(*this); }

    #define DEFINE_CLASS_TYPE(classType, type, overwritable)    static EventType GetStaticType() { return EventType::type; }\
								                                virtual EventType GetEventType() const override { return GetStaticType(); }\
								                                virtual const char* GetName() const override { return #type; }\
								                                virtual Event* Clone() const override { return new classType(*this); }\
                                                                virtual bool AllowOverwrite() const override { return overwritable; }\
                                                                virtual bool IsOverwritable(const Event* e) const override { return overwritable; }

    #define RB_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

    class Event
    {
    public:
        Event();
        virtual ~Event();

        virtual EventType	GetEventType()		            const = 0;
        virtual const char* GetName()			            const = 0;
        virtual int			GetCategoryFlags()	            const = 0;
        virtual Event*      Clone()				            const = 0;
        virtual bool        AllowOverwrite()                const = 0;
        virtual bool		IsOverwritable(const Event* e)	const = 0;

        bool IsInCategory(const EventCategory cat) const;
    };

    class EmptyEvent : public Event
    {
    public:
        DEFINE_CLASS_TYPE(EmptyEvent, None, false)
        int GetCategoryFlags() const override { return kEventCat_None; }
    };

    template<class Type, typename Function>
    bool BindEvent(const Function& func, Event& event)
    {
        if (event.GetEventType() == Type::GetStaticType())
        {
            func(static_cast<Type&>(event));
            return true;
        }

        return false;
    }

    class EventListener;

    class EventManager
    {
    public:
        EventManager();
        ~EventManager();

        void AddListener(EventListener* listener);
        void RemoveListener(EventListener* listener);

        void InsertEvent(const Event& event);

        //Event& GetPreviousEventPerCategory(const EventCategory& cat, uint8_t index);

    private:
        List<EventListener*> m_Listeners;

        //static const int EVENT_HISTORY_COUNT = 5;

        //List<Event*> m_LastEvents;
    };

    extern EventManager* g_EventManager;

    class EventListener
    {
    public:
        // Enabling double queue's is better when processing an event on the
        // listener that can take a very long time (maybe even block). 
        // This way we won't block the main thread.
        EventListener(int category, bool double_queue = false);
        virtual ~EventListener();

        bool ListensToCategory(const EventCategory cat) const
        {
            return (m_ListenerCategory & cat) > 0;
        }

        void ProcessEvents();

    private:
        void AddEvent(const Event& e);

        // Returns false when the event should be processed a next time
        virtual bool OnEvent(Event& event) = 0;

        int		            m_ListenerCategory;
        bool                m_DoubleQueue;
        bool                m_QueueCycle;
        List<Event*>		m_QueuedEvents0;
        List<Event*>		m_QueuedEvents1;
        CRITICAL_SECTION	m_CS;

        friend class EventManager;
    };
}