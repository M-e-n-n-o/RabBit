#include "RabBitCommon.h"
#include "Smaa.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"

#include "SmaaAreaTex.h"
#include "SmaaSearchTex.h"

#include "codeGen/ShaderDefines.h"

using namespace RB::Graphics::Shader;

namespace RB::Graphics
{
    struct SmaaEntry : public RenderPassEntry
    {
    };

    SmaaPass::SmaaPass()
        : m_AreaTex(nullptr)
        , m_SearchTex(nullptr)
    {
    }

    RenderPassConfig SmaaPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const SmaaSettings& s = (const SmaaSettings&)setting;

        return RenderPassConfig
        {
            // Dependencies
            {
                RenderTextureInputDesc{ "Color", -1 }
            },

            // Working textures
            {
                RenderResourceDesc {
                    .name = "SMAA Edges",
                    .format = RenderResourceFormat::RG8_UNORM,
                    .type = RenderResourcePassType::Tex2D,
                    .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                    .flags = kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph
                },
                RenderResourceDesc {
                    .name = "SMAA Blend",
                    .format = RenderResourceFormat::RGBA8_UNORM,
                    .type = RenderResourcePassType::Tex2D,
                    .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                    .flags = kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph
                }
            },

            // Output textures
            {
                RenderResourceDesc {
                    .name = "ColorAA",
                    .format = RenderResourceFormat::RGBA8_UNORM,
                    .type = RenderResourcePassType::Tex2D,
                    .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                    .flags = kRTFlag_AllowRenderTarget
                }
            },

            // Async compute compatible
            false
        };
    }

    RenderPassEntry* SmaaPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator)
    {
        SmaaEntry* e = allocator->Allocate<SmaaEntry>();
        return e;
    }

    void SmaaPass::Render(RenderPassInput& i)
    {
        if (m_AreaTex == nullptr || m_SearchTex == nullptr)
        {
            m_AreaTex = Texture2D::Create("SMAA Area", SmaaAreaTexData, sizeof(SmaaAreaTexData), RenderResourceFormat::BC5_UNORM, SmaaAreaTexWidth, SmaaAreaTexHeight, false, false);
            m_SearchTex = Texture2D::Create("SMAA Search", SmaaSearchTexData, sizeof(SmaaSearchTexData), RenderResourceFormat::BC4_UNORM, SmaaSearchTexWidth, SmaaSearchTexHeight, false, false);
        }

        RenderResource* input_tex  = i.dependencyRes[0];
        RenderResource* output_tex = i.outputRes[0];
        RenderResource* edges_tex  = i.workingRes[0];
        RenderResource* blend_tex  = i.workingRes[1];

        static const float triangle_data[] =
        {
            // Pos          UV
            -1.0f, -1.0f,   0.0f,  1.0f,
            -1.0f,  3.0f,   0.0f, -1.0f,
             3.0f, -1.0f,   2.0f,  1.0f 
        };

        auto triangle_vb = VertexBuffer::Create("Text Element", TopologyType::TriangleList, triangle_data, sizeof(float) * 4, sizeof(triangle_data), true);

        i.viewContext->SetFrameConstants(SMAAGlobals_FC, i.ri);

        i.ri->SetBlendMode(BlendMode::None);
        i.ri->SetCullMode(CullMode::Back);
        i.ri->SetDepthMode(DepthMode::PassAll, false, false);

        // Edge detection
        {
            i.ri->SetVertexShader(VS_SmaaEdgeDetection);
            i.ri->SetPixelShader(PS_SmaaEdgeDetection);

            i.ri->SetShaderResourceInput(PsSmaaEdgeDetection_ColorTex, input_tex);
            i.ri->PushRenderTarget(edges_tex);

            i.ri->SetVertexBuffer(triangle_vb.get());

            i.ri->Draw();
        }

        i.ri->ClearResourceInputs();

        // Weight calculation
        {
            i.ri->SetVertexShader(VS_SmaaBlendWeight);
            i.ri->SetPixelShader(PS_SmaaBlendWeight);

            i.ri->SetShaderResourceInput(PsSmaaBlendWeight_EdgesTex, edges_tex);
            i.ri->SetShaderResourceInput(PsSmaaBlendWeight_AreaTex, m_AreaTex.get());
            i.ri->SetShaderResourceInput(PsSmaaBlendWeight_SearchTex, m_SearchTex.get());
            i.ri->PushRenderTarget(blend_tex);

            i.ri->SetVertexBuffer(triangle_vb.get());

            i.ri->Draw();
        }

        i.ri->ClearResourceInputs();

        // Neighbor blend
        {
            i.ri->SetVertexShader(VS_SmaaNeighborBlending);
            i.ri->SetPixelShader(PS_SmaaNeighborBlending);

            i.ri->SetShaderResourceInput(PsSmaaNeighborBlending_ColorTex, input_tex);
            i.ri->SetShaderResourceInput(PsSmaaNeighborBlending_BlendTex, blend_tex);
            i.ri->PushRenderTarget(output_tex);

            i.ri->SetVertexBuffer(triangle_vb.get());

            i.ri->Draw();
        }
    }
}