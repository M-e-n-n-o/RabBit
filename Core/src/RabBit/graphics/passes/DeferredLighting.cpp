#include "RabBitCommon.h"
#include "DeferredLighting.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Transform.h"

#include "graphics/shaders/shared/Common.h"
#include "graphics/codeGen/ShaderDefines.h"

namespace RB::Graphics
{
    struct DeferredLightingEntry : public RenderPassEntry
    {
    };

    RenderPassConfig DeferredLightingPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const DeferredLightingSettings& s = (const DeferredLightingSettings&)setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"GBuffer Color"},
                    RenderTextureInputDesc{"GBuffer Normal"}
                },
                2,

                // Working textures
                {},
                0,

                // Output textures
                {
                    RenderTextureDesc{"Lit",  RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget},
                },
                1,

                // Async compute compatible
                false
            });
    }

    RenderPassEntry* DeferredLightingPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene)
    {
        // Just create an empty entry
        DeferredLightingEntry* entry = new DeferredLightingEntry();
        return entry;
    }

    void DeferredLightingPass::Render(RenderPassInput& inputs)
    {
        //static_assert(false);
        // TODO
        // - Implement a simple apply lighting pass
        // - Add this new pass to the render graph
    }
}