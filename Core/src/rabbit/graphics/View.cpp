#include "RabBitCommon.h"
#include "View.h"
#include "RenderInterface.h"

#include "shaders/shared/Common.h"

using namespace RB::Math;

namespace RB::Graphics
{
    void ViewContext::SetFrameConstants(uint32_t slot, RenderInterface* render_interface) const
    {
        SetFrameConstants(slot, render_interface, viewport, viewFrustum);
    }

    void ViewContext::SetFrameConstants(uint32_t slot, RenderInterface* render_interface, Viewport vp, Frustum frustum) const
    {
        Shader::FrameConstants constants;
        constants.worldToViewMat = frustum.GetWorldToViewMatrix();
        constants.viewToWorldMat = frustum.GetViewToWorldMatrix();
        constants.viewToClipMat  = frustum.GetViewToClipMatrix();
        constants.clipToViewMat  = frustum.GetClipToViewMatrix();
        constants.dimensions     = Float4(vp.width, vp.height, 1.0f / (float)vp.width, 1.0f / (float)vp.height);

        render_interface->SetConstantShaderData(slot, &constants, sizeof(constants));
    }
}