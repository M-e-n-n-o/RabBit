#include "RabBitCommon.h"
#include "ToneMapping.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"

#include "graphics/shaders/shared/Common.h"
#include "codeGen/ShaderDefines.h"

using namespace RB::Graphics::Shader;

namespace RB::Graphics
{
    struct ToneMappingEntry : public RenderPassEntry
    {
    };

    RenderPassConfig ToneMappingPass::GetConfiguration(const RenderPassSettings* setting)
    {
        m_Settings = *(const ToneMappingSettings*)setting;

        return RenderPassConfig
        {
            // Dependencies
            {
                RenderTextureInputDesc{ "Color", 0 }
            },

            // Working textures
            {
            },

            // Output textures
            {
                RenderResourceDesc {
                    .name = "ColorOut",
                    .format = RenderResourceFormat::RGBA8_UNORM,
                    .type = RenderResourcePassType::Tex2D,
                    .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                    .flags = kRTFlag_AllowRandomReadWrites
                }
            },

            // Async compute compatible
            false
        };
    }

    RenderPassEntry* ToneMappingPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator)
    {
        ToneMappingEntry* e = allocator->Allocate<ToneMappingEntry>();
        return e;
    }

    void ToneMappingPass::Render(RenderPassInput& i)
    {
        i.ri->SetComputeShader(CS_ToneMap);

        Shader::ToneMapCB cb = {};
        cb.clearColor       = m_Settings.applyClearColor ? i.viewContext->clearColor : Math::Float4(0);
        cb.brightnessValue  = m_Settings.brightness;
        cb.gammaValue       = m_Settings.applyGamma ? 2.2f : 1.0f;

        i.ri->SetConstantShaderData(ApplyLightingGlobals_ApplyLighting, &cb, sizeof(Shader::ToneMapCB));

        i.ri->SetRandomReadWriteInput(CsApplyLightingDeferred_Output, i.outputRes[0]);

        i.ri->Dispatch(ALIGN_8(i.viewContext->viewport.width) / 8, ALIGN_8(i.viewContext->viewport.height) / 8, 1);
    }
}