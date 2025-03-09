#pragma once

#include "Event.h"

namespace RB::Input::Events
{
    class ApplicationEvent : public Event
    {
    public:
        ApplicationEvent() {}
        virtual ~ApplicationEvent() = default;

        virtual EventType GetEventType() const override = 0;
        virtual const char* GetName() const override = 0;

        int GetCategoryFlags() const override { return kEventCat_Application; }

    private:
        void* m_WindowHandle;
    };

    class AppChangedSettingsEvent : public ApplicationEvent
    {
    public:
        AppChangedSettingsEvent() {}

        DEFINE_CLASS_TYPE(AppChangedSettingsEvent, AppSettingsChanged, true)
    };
}