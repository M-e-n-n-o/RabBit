#pragma once

#include "Event.h"
#include "app/Settings.h"

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

    class GraphicsSettingsChangedEvent : public ApplicationEvent
    {
    public:
        GraphicsSettingsChangedEvent(const GraphicsSettings& new_settings) : m_NewSettings(new_settings) {}

        const GraphicsSettings& GetSettings() const { return m_NewSettings; }

        DEFINE_CLASS_TYPE(GraphicsSettingsChangedEvent, GraphicsSettingsChanged, true)

    private:
        GraphicsSettings m_NewSettings;
    };
}