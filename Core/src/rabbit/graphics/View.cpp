#include "RabBitCommon.h"
#include "View.h"
#include "RenderInterface.h"

// Shader code
#include "shaders/shared/Common.h"

using namespace RB::Math;

namespace RB::Graphics
{
    void ViewContext::SetFrameConstants(RenderInterface* render_interface) const
    {
        SetFrameConstants(render_interface, viewport, viewFrustum);
    }

    void ViewContext::SetFrameConstants(RenderInterface* render_interface, Viewport vp, Frustum frustum) const
    {
        Float4x4 world_to_view = frustum.GetWorldToViewMatrix();

        Float4x4 view_to_world = frustum.GetWorldToViewMatrix();
        view_to_world.Invert();

        Float4x4 view_to_clip = frustum.GetViewToClipMatrix();

        Float4x4 clip_to_view = frustum.GetViewToClipMatrix();
        clip_to_view.InvertProjection();

        FrameConstants constants;
        constants.worldToViewMat    = world_to_view;
        constants.viewToWorldMat    = view_to_world;
        constants.viewToClipMat     = view_to_clip;
        constants.clipToViewMat     = clip_to_view;
        constants.dimensions        = Float4(vp.width, vp.height, 1.0f / (float)vp.width, 1.0f / (float)vp.height);

        render_interface->SetConstantShaderData(kFrameConstantsCB, &constants, sizeof(constants));
    }
}