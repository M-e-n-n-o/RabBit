#include "RabBitCommon.h"
#include "DeferredLighting.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"
#include "graphics/ResourceDefaults.h"

#include "entity/Scene.h"
#include "entity/components/Light.h"
#include "graphics/shaders/shared/Common.h"
#include "codeGen/ShaderDefines.h"

using namespace RB::Graphics::Shader;

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
                    RenderTextureInputDesc{"ShadowMap", -1}  // Optional
                },

                // Working textures
                {},

                // Output textures
                {
                    RenderResourceDesc {
                        .name     = "Lit",
                        .format   = RenderResourceFormat::RGBA32_FLOAT,
                        .type     = RenderResourcePassType::Tex2D,
                        .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                        .flags    = kRTFlag_AllowRandomReadWrites | kRTFlag_ClearBeforeGraph
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
        inputs.viewContext->SetFrameConstants(ApplyLightingGlobals_FC, inputs.ri);

        inputs.ri->SetComputeShader(CS_ApplyLightingDeferred);
        
        inputs.ri->SetShaderResourceInput(CsApplyLightingDeferred_Gbuf0, inputs.dependencyRes[0]);
        inputs.ri->SetShaderResourceInput(CsApplyLightingDeferred_Gbuf1, inputs.dependencyRes[1]);

        DeferredLightingEntry* entry = (DeferredLightingEntry*)inputs.entryContext;

        Shader::ApplyLightingCB cb = {};
        cb.light.direction = entry->light.GetDirection();
        cb.light.color     = entry->light.GetColor();

        RenderResource* shadow_map = inputs.dependencyRes[2];

        if (shadow_map)
        {
            Texture* csm = (Texture*)shadow_map;
            cb.cascades = Math::Min(csm->GetArraySize(), (uint32_t)_countof(cb.shadowVPs));

            RB_ASSERT(LOGTAG_GRAPHICS, csm->GetArraySize() <= _countof(cb.shadowVPs), "Need to increase the max shadow slices in ApplyLightingCB");

            for (int i = 0; i < cb.cascades; ++i)
            {
                float split;
                const auto frustum = entry->light.CalculateFrustum(entry->camera, entry->cameraTransform, i, cb.cascades, &split);

                cb.shadowVPs[i]         = frustum.GetWorldToViewMatrix() * frustum.GetViewToClipMatrix();
                cb.cascadeSplits.arr[i] = split;
            }

            inputs.ri->SetShaderResourceInput(CsApplyLightingDeferred_ShadowSlices, shadow_map);
        }
        else
        {
            cb.cascades = 0;
            cb.shadowVPs[0] = Math::Float4x4();

            inputs.ri->SetShaderResourceInput(CsApplyLightingDeferred_ShadowSlices, g_TexDefaultWhite.get());
        }

        inputs.ri->SetConstantShaderData(ApplyLightingGlobals_ApplyLighting, &cb, sizeof(Shader::ApplyLightingCB));

        inputs.ri->SetRandomReadWriteInput(CsApplyLightingDeferred_Output, inputs.outputRes[0]);

        // TODO: Make this a dispatch indirect per BRDF type so that the shader doesn't diverge as much 
        // (cause it currently early outs if it doesn't have to shader)
        inputs.ri->Dispatch(ALIGN_8(inputs.viewContext->viewport.width) / 8, ALIGN_8(inputs.viewContext->viewport.height) / 8, 1);
    }
}