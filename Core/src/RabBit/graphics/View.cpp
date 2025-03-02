#include "RabBitCommon.h"
#include "View.h"
#include "RenderInterface.h"

// Shader code
#include "shaders/shared/Common.h"

namespace RB::Graphics
{
    void ViewContext::SetFrameConstants(RenderInterface* render_interface) const
    {
        FrameConstants constants;
        constants.worldToViewMat    = viewFrustum.GetWorldToViewMatrix();
        constants.viewToClipMat     = viewFrustum.GetViewToClipMatrix();
        constants.viewToClipMat.Transpose();

        render_interface->SetConstantShaderData(kFrameConstantsCB, &constants, sizeof(constants));
    }
}