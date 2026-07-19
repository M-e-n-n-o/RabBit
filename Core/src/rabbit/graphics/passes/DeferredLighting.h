#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct DeferredLightingSettings : public RenderPassSettings
    {
        // Choose between Blinn-Phong or PBR lighting
    };

    class DeferredLightingPass : public RenderPass
    {
    public:
        const char* GetName() override { return "DeferredLighting"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings* settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;
    };
}