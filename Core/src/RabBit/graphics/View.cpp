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
        constants.viewToWorldMat    = constants.worldToViewMat;
        constants.viewToWorldMat.Invert();
        constants.viewToClipMat     = viewFrustum.GetViewToClipMatrix();
        constants.viewToClipMat.Transpose();
        constants.clipToViewMat     = constants.viewToClipMat;
        constants.clipToViewMat.Invert();

        constants.dimensions        = Math::Float4(viewport.width, viewport.height, 1.0f / (float)viewport.width, 1.0f / (float)viewport.height);

        render_interface->SetConstantShaderData(kFrameConstantsCB, &constants, sizeof(constants));
    }
}