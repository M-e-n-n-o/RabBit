#include "RabBitCommon.h"
#include "GBuffer.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Transform.h"

#include "graphics/shaders/shared/Common.h"
#include "graphics/codeGen/ShaderDefines.h"

using namespace RB::Entity;

namespace RB::Graphics
{
    struct GBufferEntry : public RenderPassEntry
    {
        struct ModelEntry
        {
            VertexBuffer*   vb;
            IndexBuffer*    ib;
            Texture*        texture;
            Math::Float4x4	modelMatrix;
        };

        ModelEntry*         entries;
        uint32_t            entryCount;

        ~GBufferEntry()
        {
        }
    };

    RenderPassConfig GBufferPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const GBufferSettings& s = (const GBufferSettings&) setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"Depth", true, 2}, // In-/output
                },
                1,

                // Working textures
                {},
                0,

                // Output textures
                {
                    RenderTextureDesc{"GBuffer Color",  RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph },
                    RenderTextureDesc{"GBuffer Normal", RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph },
                    RenderTextureDesc{"Hyper Depth",    RenderResourceFormat::D32_FLOAT,          kRTSize_Full, kRTSize_Full, kRTFlag_ClearBeforeGraph  },
                },
                3,

                // Async compute compatible
                false
            });
    }

    RenderPassEntry* GBufferPass::SubmitEntry(const ViewContext* view_context, FrameAllocator* allocator, const Scene* const scene)
    {
        auto mesh_renderers = scene->GetComponentsWithTypeOf<MeshRenderer>();

        GBufferEntry::ModelEntry* entries = allocator->Allocate<GBufferEntry::ModelEntry>(mesh_renderers.size());

        uint32_t total_entries = 0;

        for (int i = 0; i < mesh_renderers.size(); ++i)
        {
            const MeshRenderer* mesh_renderer = (const MeshRenderer*)mesh_renderers[i];
            const Mesh* mesh = mesh_renderer->GetMesh();
            const Material* mat = mesh_renderer->GetMaterial();

            auto vertex_pairs = mesh->GetVertexPairs();

            for (int vp_idx = 0; vp_idx < vertex_pairs.size(); vp_idx++)
            {
                const Mesh::VertexPair& vp = vertex_pairs[vp_idx];

                if (!vp.vertexBuffer->ReadyToRender() || 
                    (vp.indexBuffer && !vp.indexBuffer->ReadyToRender()) ||
                    !mat->GetTexture()->ReadyToRender())
                {
                    continue;
                }

                const Transform* transform = mesh_renderer->GetGameObject()->GetComponent<Transform>();

                if (transform == nullptr)
                {
                    continue;
                }

                GBufferEntry::ModelEntry entry = {};
                entry.vb            = vp.vertexBuffer;
                entry.ib            = vp.indexBuffer;
                entry.texture       = mat->GetTexture();
                entry.modelMatrix   = transform->GetLocalToWorldMatrix();

                entries[total_entries] = entry;
                total_entries++;
            }
        }

        if (total_entries == 0)
        {
            return nullptr;
        }

        GBufferEntry* entry = new GBufferEntry();
        entry->entries      = entries;
        entry->entryCount   = total_entries;

        return entry;
    }

    void GBufferPass::Render(RenderPassInput& in)
    {
        in.ri->SetVertexShader(VS_Gbuffer);
        in.ri->SetPixelShader(PS_Gbuffer);

        in.ri->SetBlendMode(BlendMode::None);
        in.ri->SetCullMode(CullMode::Back);
        in.ri->SetDepthMode(DepthMode::PassCloser, true, in.viewContext->viewFrustum.IsReversedDepth());

        RenderTargetBundle bundle = {};
        bundle.colorTargetsCount  = 2;
        bundle.colorTargets[0]    = (Texture2D*)in.outputTextures[0];
        bundle.colorTargets[1]    = (Texture2D*)in.outputTextures[1];
        bundle.depthStencilTarget = (Texture2D*)in.outputTextures[2];

        in.ri->SetRenderTarget(&bundle);

        GBufferEntry* entry = (GBufferEntry*)in.entryContext;

        // Set the frame constants
        in.viewContext->SetFrameConstants(in.ri);

        for (int i = 0; i < entry->entryCount; ++i)
        {
            GBufferEntry::ModelEntry& model_entry = entry->entries[i];

            in.ri->SetVertexBuffer(model_entry.vb);

            if (model_entry.ib)
            {
                in.ri->SetIndexBuffer(model_entry.ib);
            }

            in.ri->SetConstantShaderData(kInstanceCB, &model_entry.modelMatrix, sizeof(model_entry.modelMatrix));

            in.ri->SetShaderResourceInput(model_entry.texture, 1);

            in.ri->Draw();
        }
    }
}