#pragma once

#include "Frustum.h"
#include "math/Vector.h"

#include "entity/components/Camera.h"
#include "entity/components/Transform.h"

namespace RB::Graphics
{
    class RenderInterface;
    class Texture2D;

    class Viewport
    {
    public:
        uint32_t     left;
        uint32_t     top;
        uint32_t     width;
        uint32_t     height;
    };

    class ViewContext
    {
    public:
        bool                enabled;
        bool                isOffscreen;
        uint32_t            windowIndex;
        Shared<Texture2D>   finalColorTarget;
        Math::Float4        clearColor;
        uint32_t            renderGraphType;
        uint32_t            renderGraphSizeID;

        // RenderPasses can change the properties of the viewport if needed
        Viewport     viewport;
        Frustum      viewFrustum;

        // The camera this ViewContext is linked to
        Entity::Camera camera;
        Entity::Transform cameraTransform;

        void SetFrameConstants(uint32_t slot, RenderInterface* render_interface) const;
        void SetFrameConstants(uint32_t slot, RenderInterface* render_interface, Viewport vp, Frustum frustum) const;
    };
}