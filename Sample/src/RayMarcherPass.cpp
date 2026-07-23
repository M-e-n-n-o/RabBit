#include "RabBitCommon.h"
#include "RayMarcherPass.h"

#include "entity/Scene.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "codeGen/ShaderDefines.h"

#include "shaders/Shared.h"

using namespace RB::Graphics::Shader;

using namespace RB;
using namespace RB::Graphics;
using namespace RB::Entity;


struct RayMarcherRenderEntry : public RenderPassEntry
{
};

RayMarcherPass::RayMarcherPass()
    : m_CollisionBuffer(nullptr)
    , m_Readback(nullptr)
{
}

RenderPassConfig RayMarcherPass::GetConfiguration(const RenderPassSettings* setting)
{
    return RenderPassConfig
    {
        // Dependencies
        {
            RenderTextureInputDesc{"GBuffer0",  0},
            RenderTextureInputDesc{"GBuffer1",  1},
        },

        // Working textures
        {},

        // Output textures
        {
            RenderResourceDesc {
                .name     = "GBuffer0",
                .format   = RenderResourceFormat::RGBA32_FLOAT,
                .type     = RenderResourcePassType::Tex2D,
                .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                .flags    = kRTFlag_AllowRandomReadWrites
            },

            RenderResourceDesc {
                .name     = "GBuffer1",
                .format   = RenderResourceFormat::RGBA32_FLOAT,
                .type     = RenderResourcePassType::Tex2D,
                .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                .flags    = kRTFlag_AllowRandomReadWrites
            },
        },

        // Async compute compatible
        false
    };
}

RenderPassEntry* RayMarcherPass::SubmitEntry(const ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
{
    RayMarcherRenderEntry* entry = allocator->Allocate<RayMarcherRenderEntry>();
    return entry;
}

void RayMarcherPass::Render(RenderPassInput& in)
{
    if (m_CollisionBuffer == nullptr)
    {
        m_CollisionBuffer = GenericBuffer::Create("Collision Buffer", sizeof(float), MAX_COLLISION_CHECKS, true);
        m_Readback = ReadbackBuffer::Create("Collision readbacks", sizeof(float) * MAX_COLLISION_CHECKS);
    }

    {
        in.ri->SetComputeShader(CS_RayMarching);

        in.viewContext->SetFrameConstants(RayMarcherGlobals_FC, in.ri);

        in.ri->SetRandomReadWriteInput(CsRayMarching_Gbuf0, in.outputRes[0]);
        in.ri->SetRandomReadWriteInput(CsRayMarching_Gbuf1, in.outputRes[1]);

        in.ri->Dispatch(ALIGN_8(in.viewContext->viewport.width) / 8, ALIGN_8(in.viewContext->viewport.height) / 8, 1);
    }

    in.ri->ClearResourceInputs();

    {
        in.ri->SetComputeShader(CS_CollisionCheck);

        CollisionCheckCB data = {};
        data.points[0] = Math::Float4(in.viewContext->cameraTransform.position);
        data.maxPoints = 1;

        in.ri->SetConstantShaderData(RayMarcherGlobals_CollisionChecks, &data, sizeof(CollisionCheckCB));

        in.ri->SetRandomReadWriteInput(CsCollisionCheck_Distances, m_CollisionBuffer.get());

        in.ri->Dispatch(1, 1, 1);

        in.ri->Readback(m_CollisionBuffer.get(), m_Readback.get());

        float* distances = ALLOC_STACKC(float, MAX_COLLISION_CHECKS);
        if (m_Readback->GetData(distances))
        {
            RB_LOG("Distance: %f", distances[0]);
        }
    }
}