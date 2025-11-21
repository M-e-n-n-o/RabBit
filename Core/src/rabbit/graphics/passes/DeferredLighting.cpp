#include "RabBitCommon.h"
#include "DeferredLighting.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"
#include "graphics/ResourceDefaults.h"

#include "entity/Scene.h"
#include "entity/components/Light.h"
#include "graphics/shaders/shared/Common.h"
#include "graphics/shaders/shared/ConstantBuffers.h"
#include "graphics/codeGen/ShaderDefines.h"

namespace RB::Graphics
{
    struct DeferredLightingEntry : public RenderPassEntry
    {
        Math::Float4x4 shadowVP;
        Math::Float3   direction;
        Math::Float3   color;
    };

    RenderPassConfig DeferredLightingPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const DeferredLightingSettings& s = (const DeferredLightingSettings&)setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"GBuffer0",  -1},
                    RenderTextureInputDesc{"GBuffer1",  -1},
                    RenderTextureInputDesc{"ShadowMap", -1}
                },

                // Working textures
                {},

                // Output textures
                {
                    RenderTextureDesc{"Lit",  RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRandomReadWrites},
                },

                // Async compute compatible
                false
            });
    }

    RenderPassEntry* DeferredLightingPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator)
    {
        const auto& list = scene->GetComponentsWithTypeOf<Entity::DirectionalLight>();

        DeferredLightingEntry* entry = new DeferredLightingEntry();

        if (list.empty())
        {
            entry->direction = Math::Float3(0, -1, 0);
            entry->color     = Math::Float3(0, 0, 0);
            // TODO Fill in a shadowVP that causes the ndc.z te be smaller than 1
        }
        else
        {
            const auto* light = (Entity::DirectionalLight*)list[0];

            auto frustum = light->CalculateFrustum(*view_context->camera, *view_context->cameraTransform);

            entry->shadowVP  = frustum.GetWorldToViewMatrix() * frustum.GetViewToClipMatrix();
            entry->direction = light->GetDirection();
            entry->color     = light->GetColor();
        }

        return entry;
    }

    void DeferredLightingPass::Render(RenderPassInput& inputs)
    {
        inputs.viewContext->SetFrameConstants(inputs.ri);

        inputs.ri->SetComputeShader(CS_ApplyLightingDeferred);
        
        inputs.ri->SetShaderResourceInput(inputs.dependencyTextures[0], 0);
        inputs.ri->SetShaderResourceInput(inputs.dependencyTextures[1], 1);

        if (inputs.dependencyTextures[2])
            inputs.ri->SetShaderResourceInput(inputs.dependencyTextures[2], 2);
        else
            inputs.ri->SetShaderResourceInput(g_TexDefaultWhite.get(), 2);

        inputs.ri->SetRandomReadWriteInput(inputs.outputTextures[0], 0);

        DeferredLightingEntry* entry = (DeferredLightingEntry*)inputs.entryContext;

        ApplyLightingCB cb = {};
        cb.light.direction = entry->direction;
        cb.light.color     = entry->color;
        cb.shadowVP        = entry->shadowVP;

        inputs.ri->SetConstantShaderData(kInstanceCB, &cb, sizeof(ApplyLightingCB));

        // TODO: Make this a dispatch indirect per BRDF type if the code paths start to diverge
        inputs.ri->Dispatch(ALIGN_8(inputs.viewContext->viewport.width) / 8, ALIGN_8(inputs.viewContext->viewport.height) / 8, 1);
    }
}