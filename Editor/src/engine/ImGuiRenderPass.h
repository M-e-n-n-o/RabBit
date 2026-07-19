#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderPass.h"

namespace Editor
{
    struct ImGuiRenderSettings : public RB::Graphics::RenderPassSettings
    {
        // No settings
    };

    class ImGuiRenderPass : public RB::Graphics::RenderPass
    {
    public:
        const char* GetName() override { return "ImGuiRenderer"; }

        RB::Graphics::RenderPassConfig GetConfiguration(const RB::Graphics::RenderPassSettings* settings) override;

        RB::Graphics::RenderPassEntry* SubmitEntry(const RB::Graphics::ViewContext* view_context, const RB::Entity::Scene* const scene, RB::FrameAllocator* allocator) override;

        void Render(RB::Graphics::RenderPassInput& inputs) override;
    };
}