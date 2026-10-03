#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct SkinningSettings : public RenderPassSettings
    {
        // No settings
    };

    class SkinningPass : public RenderPass
    {
    public:
        const char* GetName() override { return "Skinning"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings* settings) override;

        RenderPassEntry* SubmitEntry(ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;
    };
}