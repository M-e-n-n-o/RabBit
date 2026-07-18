#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct ToneMappingSettings : public RenderPassSettings
    {
        bool applyClearColor;
    };

    class ToneMappingPass : public RenderPass
    {
    public:
        const char* GetName() override { return "ToneMapping"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings& settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;

    private:
        bool m_ApplyClearColor;
    };
}