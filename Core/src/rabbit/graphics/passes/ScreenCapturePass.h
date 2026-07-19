#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct ScreenCaptureSettings : public RenderPassSettings
    {
    };

    class ScreenCapturePass : public RenderPass
    {
    public:
        const char* GetName() override { return "ScreenCapture"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings* settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;
    };
}