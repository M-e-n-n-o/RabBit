#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct Overlay2DSettings : public RenderPassSettings
    {
        // No settings
    };

    class Overlay2DPass : public RenderPass
    {
    public:
        const char* GetName() override { return "Overlay2D"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings& settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene) override;

        void Render(RenderPassInput& inputs) override;
    };
}