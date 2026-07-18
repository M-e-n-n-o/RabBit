#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct SmaaSettings : public RenderPassSettings
    {
        
    };

    class SmaaPass : public RenderPass
    {
    public:
        SmaaPass();

        const char* GetName() override { return "SMAA"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings& settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;

    private:
        Shared<Texture2D> m_AreaTex;
        Shared<Texture2D> m_SearchTex;
    };
}