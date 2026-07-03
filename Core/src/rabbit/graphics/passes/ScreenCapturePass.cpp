#include "RabBitCommon.h"
#include "ScreenCapturePass.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"

#include "entity/Scene.h"
#include "entity/components/ScreenCapturer.h"

namespace RB::Graphics
{
    struct ScreenCaptureEntry : public RenderPassEntry
    {
        Shared<ReadbackBuffer>     readback;
    };

    RenderPassConfig ScreenCapturePass::GetConfiguration(const RenderPassSettings& setting)
    {
        const ScreenCaptureSettings& s = (const ScreenCaptureSettings&)setting;

        return RenderPassConfig
        {
            // Dependencies
            {
                RenderTextureInputDesc{"FinalColor", 0}
            },

            // Working textures
            {
            },

            // Output textures
            {
                RenderResourceDesc {
                    .name = "FinalColor",
                    .format = RenderResourceFormat::R32G32B32A32_FLOAT,
                    .type = RenderResourcePassType::Tex2D,
                    .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                    .flags = kRTFlag_AllowRenderTarget
                }
            },

            // Async compute compatible
            false
        };
    }

    RenderPassEntry* ScreenCapturePass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator)
    {
        const auto& list = scene->GetComponentsWithTypeOf<Entity::ScreenCapturer>();

        if (list.empty())
            return nullptr;

        const auto& screen_capturer = (Entity::ScreenCapturer*)list[0];

        if (!screen_capturer->ShouldMakeCapture())
            return nullptr;

        ScreenCaptureEntry* entry = allocator->Allocate<ScreenCaptureEntry>();
        entry->readback = screen_capturer->GetReadbackBuffer();
        return entry;
    }

    void ScreenCapturePass::Render(RenderPassInput& inputs)
    {
        ScreenCaptureEntry* entry = (ScreenCaptureEntry*)inputs.entryContext;

        // TODO: Should make sure that the outputted data is in sRGB format, as it is now just linear

        // Schedule a readback on the GPU
        inputs.ri->Readback(inputs.outputRes[0], entry->readback.get());
    }
}