#include "RabBitCommon.h"
#include "Smaa.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"

#include "SmaaAreaTex.h"
#include "SmaaSearchTex.h"

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
                RenderTextureInputDesc{ "Color", 0 }
            },

            // Working textures
            {
                RenderResourceDesc {
                    .name = "SMAA Edges",
                    .format = RenderResourceFormat::R8G8_UNORM,
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
                    .format = RenderResourceFormat::RGBA32_FLOAT,
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
        return nullptr;
    }

    void SmaaPass::Render(RenderPassInput& inputs)
    {
        if (m_AreaTex == nullptr || m_SearchTex == nullptr)
        {
            m_AreaTex = Texture2D::Create("SMAA Area", SmaaAreaTexData, sizeof(SmaaAreaTexData), RenderResourceFormat::BC5_UNORM, SmaaAreaTexWidth, SmaaAreaTexHeight, false, false);
            m_SearchTex = Texture2D::Create("SMAA Search", SmaaSearchTexData, sizeof(SmaaSearchTexData), RenderResourceFormat::BC4_UNORM, SmaaSearchTexWidth, SmaaSearchTexHeight, false, false);
        }


    }
}