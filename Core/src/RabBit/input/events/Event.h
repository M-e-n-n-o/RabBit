#pragma once

#include "RabBitCommon.h"

namespace RB::Input::Events
{
	enum class EventType
	{
		None = 0,
		WindowCreated, WindowCloseRequest, WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		kEventCat_None			= 0,
		kEventCat_Window		= (1 << 0),
		kEventCat_Keyboard		= (1 << 1),
		kEventCat_Mouse			= (1 << 2),
		kEventCat_MouseButton	= (1 << 3),

		kEventCat_Input			= (kEventCat_Keyboard | kEventCat_Mouse | kEventCat_MouseButton),
		kEventCat_All			= (kEventCat_Window | kEventCat_Input | kEventCat_Keyboard | kEventCat_Mouse | kEventCat_MouseButton)
	};

	#define DEFINE_CLASS_TYPE(classType, type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }\
								virtual Event* Clone() const override { return new classType(*this); }

	#define RB_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

	class Event
	{
	public:
		Event();
		virtual ~Event();

		virtual EventType	GetEventType()		const = 0;
		virtual const char* GetName()			const = 0;
		virtual int			GetCategoryFlags()	const = 0;
		virtual Event*		Clone()				const = 0;

		bool IsInCategory(const EventCategory cat) const;
	};

	class EmptyEvent : public Event
	{
	public:
		DEFINE_CLASS_TYPE(EmptyEvent, None)
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
		EventListener(EventCategory category);
		virtual ~EventListener();

		bool ListensToCategory(const EventCategory cat) const
		{
			return (m_ListenerCategory & cat) > 0;
		}

		void ProcessEvents();

	private:
		void AddEvent(const Event& e);

		virtual void OnEvent(Event& event) = 0;

		EventCategory		m_ListenerCategory;
		List<Event*>		m_QueuedEvents;
		CRITICAL_SECTION	m_CS;

		friend class EventManager;
	};
}