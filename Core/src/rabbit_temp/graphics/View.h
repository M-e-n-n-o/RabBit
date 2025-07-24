#pragma once

#include "Frustum.h"
#include "math/Vector.h"

namespace RB::Graphics
{
    class RenderInterface;
    class Texture2D;

    class Viewport
    {
    public:
        uint32_t        left;
        uint32_t        top;
        uint32_t        width;
        uint32_t        height;
    };

    class ViewContext
    {
    public:
        bool            enabled;
        bool			isOffscreenContext;
        uint32_t		windowIndex;
        Texture2D*      finalColorTarget;
        Math::Float4	clearColor;
        uint32_t        renderGraphType;

        // RenderPasses can change the properties of the viewport if needed
        Viewport        viewport;
        Frustum			viewFrustum;

        void SetFrameConstants(RenderInterface* render_interface) const;
    };
}