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
        // Just copy over all the entity info for now.
        // In future do most processing/calculations in SubmitEntry instead of Render method
        bool has_light;
        Entity::DirectionalLight light;
        Entity::Camera camera;
        Entity::Transform cameraTransform;
    };

    RenderPassConfig DeferredLightingPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const DeferredLightingSettings& s = (const DeferredLightingSettings&)setting;

        return RenderPassConfig
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
                    RenderResourceDesc {
                        .name   = "Lit",
                        .format = RenderResourceFormat::R32G32B32A32_FLOAT,
                        .type   = RenderResourcePassType::Tex2D,
                        .tex2D  = { kRTSize_Full, kRTSize_Full, 1 },
                        .flags  = kRTFlag_AllowRandomReadWrites
                    }
                },

                // Async compute compatible
                false
            };
    }

    RenderPassEntry* DeferredLightingPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator)
    {
        const auto& list = scene->GetComponentsWithTypeOf<Entity::DirectionalLight>();

        DeferredLightingEntry* entry = (DeferredLightingEntry*)allocator->Allocate(sizeof(DeferredLightingEntry));
        entry->camera           = *view_context->camera;
        entry->cameraTransform  = *view_context->cameraTransform;

        if (list.empty())
        {
            entry->has_light = false;
            entry->light     = Entity::DirectionalLight(Math::Float3(0), Math::Float3(0));
        }
        else
        {
            entry->has_light = true;
            entry->light     = *((Entity::DirectionalLight*)list[0]);
        }

        return entry;
    }

    void DeferredLightingPass::Render(RenderPassInput& inputs)
    {
        inputs.viewContext->SetFrameConstants(inputs.ri);

        inputs.ri->SetComputeShader(CS_ApplyLightingDeferred);
        
        inputs.ri->SetShaderResourceInput(inputs.dependencyRes[0], 0);
        inputs.ri->SetShaderResourceInput(inputs.dependencyRes[1], 1);

        DeferredLightingEntry* entry = (DeferredLightingEntry*)inputs.entryContext;

        ApplyLightingCB cb = {};
        cb.light.direction = entry->light.GetDirection();
        cb.light.color     = entry->light.GetColor();
        cb.slices          = 0;

        RenderResource* shadow_map = inputs.dependencyRes[2];

        if (shadow_map)
        {
            Texture* csm = (Texture*)shadow_map;
            cb.slices = Math::Min(csm->GetArraySize(), (uint32_t)_countof(cb.shadowVPs));

            RB_ASSERT(LOGTAG_GRAPHICS, csm->GetArraySize() <= _countof(cb.shadowVPs), "Need to increase the max shadow slices in ApplyLightingCB");

            for (int i = 0; i < cb.slices; ++i)
            {
                const auto frustum = entry->light.CalculateFrustum(entry->camera, entry->cameraTransform, i, cb.slices);
                cb.shadowVPs[i] = frustum.GetWorldToViewMatrix() * frustum.GetViewToClipMatrix();
            }

            inputs.ri->SetShaderResourceInput(shadow_map, 2);
        }
        else
        {
            inputs.ri->SetShaderResourceInput(g_TexDefaultWhite.get(), 2);
        }

        inputs.ri->SetConstantShaderData(kInstanceCB, &cb, sizeof(ApplyLightingCB));

        inputs.ri->SetRandomReadWriteInput(inputs.outputRes[0], 3);

        // TODO: Make this a dispatch indirect per BRDF type so that the shader doesn't diverge as much 
        // (cause it currently early outs if it doesn't have to shader)
        inputs.ri->Dispatch(ALIGN_8(inputs.viewContext->viewport.width) / 8, ALIGN_8(inputs.viewContext->viewport.height) / 8, 1);
    }
}