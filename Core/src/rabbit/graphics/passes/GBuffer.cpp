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
            Shared<VertexBuffer> vb_primary;
            Shared<VertexBuffer> vb_secondary;
            Shared<IndexBuffer>  ib;
            Shared<Texture>      texture;
            Math::Float4x4	     modelMatrix;
        };

        ModelEntry*         entries;
        uint32_t            entryCount;
    };

    RenderPassConfig GBufferPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const GBufferSettings& s = (const GBufferSettings&) setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"Depth", 2}, // In-/output
                },

                // Working textures
                {},

                // Output textures
                {
                    RenderTextureDesc{"GBuffer Color",  RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph },
                    RenderTextureDesc{"GBuffer Normal", RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph },
                    RenderTextureDesc{"Hyper Depth",    RenderResourceFormat::D32_FLOAT,          kRTSize_Full, kRTSize_Full, kRTFlag_ClearBeforeGraph  },
                },

                // Async compute compatible
                false
            });
    }

    RenderPassEntry* GBufferPass::SubmitEntry(const ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
    {
        auto mesh_renderers = scene->GetComponentsWithTypeOf<MeshRenderer>();

        uint32_t size = sizeof(GBufferEntry::ModelEntry) * mesh_renderers.size();
        GBufferEntry::ModelEntry* entries = (GBufferEntry::ModelEntry*)allocator->Allocate(size);
        memset(entries, 0, size);

        uint32_t total_entries = 0;

        for (int i = 0; i < mesh_renderers.size(); ++i)
        {
            // TODO: GameObjects that use the same static Mesh & Material should be instanced.
            // It would be a good idea to add a SetInstancedData method to the ViewContext and macro's
            // in the shaders so that it, for examply, automatically picks the correct instanced model matrix
            // in the Transform helper functions.

            const MeshRenderer*     mesh_renderer   = (const MeshRenderer*)mesh_renderers[i];
            const Mesh*             mesh            = mesh_renderer->GetMesh();
            const Material*         mat             = mesh_renderer->GetMaterial();
            const Mesh::VertexPack& vp              = mesh->GetVertexPack();

            if (!vp.primaryBuffer || !vp.primaryBuffer->ReadyToRender() ||
                (vp.secondaryBuffer && !vp.secondaryBuffer->ReadyToRender()) ||
                (vp.indexBuffer && !vp.indexBuffer->ReadyToRender()) ||
                !mat->GetTexture()->ReadyToRender())
            {
                continue;
            }

            const Transform* transform = mesh_renderer->GetGameObject()->GetComponent<Transform>();

            GBufferEntry::ModelEntry entry = {};
            entry.vb_primary    = vp.primaryBuffer;
            entry.vb_secondary  = vp.secondaryBuffer;
            entry.ib            = vp.indexBuffer;
            entry.texture       = mat->GetTexture();
            entry.modelMatrix   = transform->GetLocalToWorldMatrix();

            entries[total_entries] = entry;
            total_entries++;
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

        in.ri->PushRenderTarget(in.outputTextures[0], 0);
        in.ri->PushRenderTarget(in.outputTextures[1], 1);
        in.ri->SetDepthStencil(in.outputTextures[2]);

        GBufferEntry* entry = (GBufferEntry*)in.entryContext;

        // Set the frame constants
        in.viewContext->SetFrameConstants(in.ri);

        for (int i = 0; i < entry->entryCount; ++i)
        {
            GBufferEntry::ModelEntry& model_entry = entry->entries[i];

            RenderResource* vbos[2];
            vbos[0] = model_entry.vb_primary.get();
            if (model_entry.vb_secondary)
                vbos[1] = model_entry.vb_secondary.get();

            in.ri->SetVertexBuffers(vbos, model_entry.vb_secondary ? 2 : 1);

            if (model_entry.ib)
            {
                in.ri->SetIndexBuffer(model_entry.ib.get());
            }

            in.ri->SetConstantShaderData(kInstanceCB, &model_entry.modelMatrix, sizeof(model_entry.modelMatrix));

            in.ri->SetShaderResourceInput(model_entry.texture.get(), 1);

            in.ri->Draw();
        }
    }
}