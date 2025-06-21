#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct DeferredLightingSettings : public RenderPassSettings
    {
        // No settings
    };

    class DeferredLightingPass : public RenderPass
    {
    public:
        const char* GetName() override { return "DeferredLighting"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings& settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene) override;

        void Render(RenderPassInput& inputs) override;
    };
}