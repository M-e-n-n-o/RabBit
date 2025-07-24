#pragma once

namespace RB
{
    struct GraphicsSettings
    {
        void Validate() {}

        void Print();

        // Returns if the rendergraph has to recreate its resources on a settings change
        bool RequiresNewResources(GraphicsSettings old_settings) const { return false; }
    };
}