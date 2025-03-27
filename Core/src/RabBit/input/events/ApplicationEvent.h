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
    };

    class GraphicsSettingsChangedEvent : public ApplicationEvent
    {
    public:
        GraphicsSettingsChangedEvent(const GraphicsSettings& new_settings, const GraphicsSettings& old_settings) : m_NewSettings(new_settings), m_OldSettings(old_settings) {}

        const GraphicsSettings& GetNewSettings() const { return m_NewSettings; }
        const GraphicsSettings& GetOldSettings() const { return m_OldSettings; }

        DEFINE_CLASS_TYPE(GraphicsSettingsChangedEvent, GraphicsSettingsChanged, true)

    private:
        GraphicsSettings m_NewSettings;
        GraphicsSettings m_OldSettings;
    };
}