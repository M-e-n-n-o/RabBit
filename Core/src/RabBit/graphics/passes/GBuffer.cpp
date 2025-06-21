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
        uint32_t            totalEntries;

        ~GBufferEntry()
        {
            SAFE_FREE(entries);
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

    RenderPassEntry* GBufferPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene)
    {
        auto mesh_renderers = scene->GetComponentsWithTypeOf<Entity::MeshRenderer>();

        GBufferEntry::ModelEntry* entries = (GBufferEntry::ModelEntry*)ALLOC_HEAP(sizeof(GBufferEntry::ModelEntry) * mesh_renderers.size());

        uint32_t total_entries = 0;

        for (int i = 0; i < mesh_renderers.size(); ++i)
        {
            const Entity::MeshRenderer* mesh_renderer = (const Entity::MeshRenderer*)mesh_renderers[i];
            const Entity::Mesh* mesh = mesh_renderer->GetMesh();
            const Entity::Material* mat = mesh_renderer->GetMaterial();

            if (mesh->GetVertexBuffer()->ReadyToRender() || mat->GetTexture()->ReadyToRender())
            {
                continue;
            }

            const Entity::Transform* transform = mesh_renderer->GetGameObject()->GetComponent<Entity::Transform>();

            if (transform == nullptr)
            {
                continue;
            }

            GBufferEntry::ModelEntry entry = {};
            entry.vb            = mesh->GetVertexBuffer();
            entry.ib            = mesh->GetIndexBuffer();
            entry.texture       = mat->GetTexture();
            entry.modelMatrix   = transform->GetLocalToWorldMatrix();

            entries[total_entries] = entry;
            total_entries++;
        }

        if (total_entries == 0)
        {
            SAFE_FREE(entries);
            return nullptr;
        }

        GBufferEntry* entry = new GBufferEntry();
        entry->entries      = entries;
        entry->totalEntries = total_entries;

        return entry;
    }

    void GBufferPass::Render(RenderPassInput& in)
    {
        in.renderInterface->SetVertexShader(VS_Gbuffer);
        in.renderInterface->SetPixelShader(PS_Gbuffer);

        in.renderInterface->SetBlendMode(BlendMode::None);
        in.renderInterface->SetCullMode(CullMode::Back);
        in.renderInterface->SetDepthMode(DepthMode::PassCloser, true, in.viewContext->viewFrustum.IsReversedDepth());

        RenderTargetBundle bundle = {};
        bundle.colorTargetsCount  = 2;
        bundle.colorTargets[0]    = (Texture2D*)in.outputTextures[0];
        bundle.colorTargets[1]    = (Texture2D*)in.outputTextures[1];
        bundle.depthStencilTarget = (Texture2D*)in.outputTextures[2];

        in.renderInterface->SetRenderTarget(&bundle);

        GBufferEntry* entry = (GBufferEntry*)in.entryContext;

        // Set the frame constants
        in.viewContext->SetFrameConstants(in.renderInterface);

        for (int i = 0; i < entry->totalEntries; ++i)
        {
            GBufferEntry::ModelEntry& model_entry = entry->entries[i];

            in.renderInterface->SetVertexBuffer(model_entry.vb);

            if (model_entry.ib)
            {
                in.renderInterface->SetIndexBuffer(model_entry.ib);
            }

            in.renderInterface->SetConstantShaderData(kInstanceCB, &model_entry.modelMatrix, sizeof(model_entry.modelMatrix));

            in.renderInterface->SetShaderResourceInput(model_entry.texture, 1);

            in.renderInterface->Draw();
        }
    }
}