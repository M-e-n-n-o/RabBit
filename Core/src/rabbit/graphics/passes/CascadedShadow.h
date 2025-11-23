#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct CascadedShadowSettings : public RenderPassSettings
    {
        // Resolution & shadow slices settings
    };

    class CascadedShadowPass : public RenderPass
    {
    public:
        const char* GetName() override { return "CascadedShadow"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings& settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;

    private:
        const uint32_t m_ShadowSlices = 4;
    };
}