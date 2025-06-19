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
        /*
            When passing matrix data to HLSL, it stores it in column major order. 
            So make sure to first transpose matrices before sending them to the 
            GPU as they are stored in row major on the CPU.

            Not really sure why, but the above only seems to be necessary for the viewToClip??
        */

        Float4x4 world_to_view = viewFrustum.GetWorldToViewMatrix();
        //world_to_view.Transpose();

        Float4x4 view_to_world = viewFrustum.GetWorldToViewMatrix();
        view_to_world.Invert();
        //view_to_world.Transpose();

        Float4x4 view_to_clip = viewFrustum.GetViewToClipMatrix();
        view_to_clip.Transpose();

        Float4x4 clip_to_view = viewFrustum.GetViewToClipMatrix();
        clip_to_view.Invert();
        clip_to_view.Transpose();

        FrameConstants constants;
        constants.worldToViewMat    = world_to_view;
        constants.viewToWorldMat    = view_to_world;
        constants.viewToClipMat     = view_to_clip;
        constants.clipToViewMat     = clip_to_view;
        constants.dimensions        = Float4(viewport.width, viewport.height, 1.0f / (float)viewport.width, 1.0f / (float)viewport.height);

        render_interface->SetConstantShaderData(kFrameConstantsCB, &constants, sizeof(constants));
    }
}