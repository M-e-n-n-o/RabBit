#pragma once

#include "Event.h"

namespace RB::Events
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

    class RenderOutputChangedEvent : public ApplicationEvent
    {
    public:
        RenderOutputChangedEvent() : ApplicationEvent() {}

        DEFINE_CLASS_TYPE(RenderOutputChangedEvent, RenderOutputChanged, true)
    };
}