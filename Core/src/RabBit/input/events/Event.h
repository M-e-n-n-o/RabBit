#pragma once

namespace RB::Input::Events
{
	enum class EventType
	{
		None = 0,
		WindowCreated, WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None				= 0,
		EventCatApplication = (1 << 0),
		EventCatInput		= (1 << 1),
		EventCatKeyboard	= (1 << 2),
		EventCatMouse		= (1 << 3),
		EventCatMouseButton = (1 << 4),
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
			return GetCategoryFlags() & cat;
		}

	private:
		bool m_IsProcessed = false;
	};

	class EmptyEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(None)
		int GetCategoryFlags() const override { return None; }
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

	class EventListener
	{
	public:
		virtual ~EventListener() = default;
		virtual void OnEvent(Event& event) = 0;
	};
}