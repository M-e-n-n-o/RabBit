#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderPass.h"
#include "graphics/RenderResource.h"

struct RayMarcherSettings : public RB::Graphics::RenderPassSettings
{
    // No settings
};

class RayMarcherPass : public RB::Graphics::RenderPass
{
public:
    RayMarcherPass();

    const char* GetName() override { return "RayMarcher"; }

    RB::Graphics::RenderPassConfig GetConfiguration(const RB::Graphics::RenderPassSettings* settings) override;

    RB::Graphics::RenderPassEntry* SubmitEntry(const RB::Graphics::ViewContext* view_context, const RB::Entity::Scene* const scene, RB::FrameAllocator* allocator) override;

    void Render(RB::Graphics::RenderPassInput& inputs) override;

private:
    RB::Shared<RB::Graphics::GenericBuffer> m_CollisionBuffer;
    RB::Shared<RB::Graphics::ReadbackBuffer> m_Readback;
};