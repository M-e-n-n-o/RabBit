#pragma once

namespace Editor
{
    class WindowPanel
    {
    public:
        virtual void OnCreate() = 0;
        virtual void OnDestroy() = 0;

        virtual void OnUpdate() = 0;
    };
}