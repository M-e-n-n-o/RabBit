#include "RabBitCommon.h"
#include "ApplyLighting.h"

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
    struct ApplyLightingEntry : public RenderPassEntry
    {
    };

    RenderPassConfig ApplyLightingPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const ApplyLightingSettings& s = (const ApplyLightingSettings&)setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"GBuffer Color"},
                    RenderTextureInputDesc{"GBuffer Normal"}
                },
                0,

                // Working textures
                {},
                0,

                // Output textures
                {
                    RenderTextureDesc{"Lit",  RenderResourceFormat::R32G32B32A32_FLOAT, RTSize_Full, RTSize_Full, RTFlag_AllowRenderTarget},
                },
                1,

                // Async compute compatible
                false
            });
    }

    RenderPassEntry* ApplyLightingPass::SubmitEntry(const Entity::Scene* const scene)
    {
        // Just create an empty entry
        ApplyLightingEntry* entry = new ApplyLightingEntry();
        return entry;
    }

    void ApplyLightingPass::Render(RenderInterface* render_interface, ViewContext* view_context, RenderPassEntry* entry_context,
        RenderResource** output_textures, RenderResource** working_textures, RenderResource** dependency_textures)
    {
        static_assert(false);
        // TODO
        // - Implement a simple apply lighting pass
        // - Add this new pass to the render graph
    }
}