#pragma once

#include "RabBitCommon.h"

namespace RB::Input::Events
{
	enum class EventType
	{
		None = 0,
		WindowCreated, WindowCloseRequest, WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowRender,
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

	#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

	#define RB_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;

		void SetProcessed(bool processed) { m_IsProcessed = processed; }
		bool IsProcessed() const { return m_IsProcessed; }

		bool IsInCategory(const EventCategory cat) const
		{
			return (GetCategoryFlags() & cat) > 0;
		}

	private:
		bool m_IsProcessed = false;
	};

	class EmptyEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(None)
		int GetCategoryFlags() const override { return kEventCat_None; }
	};

	template<class Type, typename Function>
	bool BindEvent(const Function& func, Event& event)
	{
		if (!event.IsProcessed())
		{
			if (event.GetEventType() == Type::GetStaticType())
			{
				event.SetProcessed(true);
				func(static_cast<Type&>(event));
				return true;
			}
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

		void InsertEvent(Event* event);

		//Event& GetPreviousEventPerCategory(const EventCategory& cat, uint8_t index);

	private:
		List<EventListener*> m_Listeners;

		static const int EVENT_HISTORY_COUNT = 5;

		List<Event*> m_LastEvents;
	};

	extern EventManager* g_EventManager;

	class EventListener
	{
	public:
		EventListener(EventCategory category)
			: m_ListenerCategory(category)
		{
			g_EventManager->AddListener(this);
		}

		virtual ~EventListener()
		{
			g_EventManager->RemoveListener(this);
		}

		virtual void OnEvent(Event& event) = 0;

		bool ListensToCategory(const EventCategory cat) const
		{
			return (m_ListenerCategory & cat) > 0;
		}

	private:
		EventCategory m_ListenerCategory;
	};
}