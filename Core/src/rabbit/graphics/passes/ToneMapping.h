#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
    struct ToneMappingSettings : public RenderPassSettings
    {
        float brightness = 1.0f;
        bool  applyGamma = true;
        bool  applyClearColor = false;

        ToneMappingSettings(float brightness = 1.0f, float applyGamma = true, float applyClearColor = false)
            : brightness(brightness)
            , applyGamma(applyGamma)
            , applyClearColor(applyClearColor)
        {}
    };

    class ToneMappingPass : public RenderPass
    {
    public:
        const char* GetName() override { return "ToneMapping"; }

        RenderPassConfig GetConfiguration(const RenderPassSettings* settings) override;

        RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene, FrameAllocator* allocator) override;

        void Render(RenderPassInput& inputs) override;

    private:
        ToneMappingSettings m_Settings;
    };
}