#include "RabBitCommon.h"
#include "GBuffer.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Transform.h"

#include "graphics/shaders/shared/Common.h"
#include "codeGen/ShaderDefines.h"

using namespace RB::Entity;
using namespace RB::Graphics::Shader;

namespace RB::Graphics
{
    struct GBufferEntry : public RenderPassEntry
    {
        struct ModelEntry
        {
            Shared<VertexBuffer> vb_primary;
            Shared<VertexBuffer> vb_secondary;
            Shared<IndexBuffer>  ib;
            Shared<Texture>      albedoTex;
            Shared<Texture>      normalTex;
            Math::Float4x4	     modelMatrix;
        };

        ModelEntry*         entries;
        uint32_t            entryCount;
    };

    RenderPassConfig GBufferPass::GetConfiguration(const RenderPassSettings* setting)
    {
        const GBufferSettings* s = (const GBufferSettings*) setting;

        return RenderPassConfig
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"Depth", 2}, // In-/output
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
                        .flags    = kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph
                    },

                    RenderResourceDesc {
                        .name     = "GBuffer1",
                        .format   = RenderResourceFormat::RGBA32_FLOAT,
                        .type     = RenderResourcePassType::Tex2D,
                        .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                        .flags    = kRTFlag_AllowRenderTarget | kRTFlag_ClearBeforeGraph
                    },

                    RenderResourceDesc {
                        .name     = "Hyper Depth",
                        .format   = RenderResourceFormat::D32_FLOAT,
                        .type     = RenderResourcePassType::Tex2D,
                        .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                        .flags    = kRTFlag_ClearBeforeGraph
                    }
                },

                // Async compute compatible
                false,

                kViewerListPerspective_Camera
            };
    }

    RenderPassEntry* GBufferPass::SubmitEntry(ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
    {
        auto renderables = view_context->viewerLists[kViewerListPerspective_Camera]->GetViewables();

        if (renderables.empty())
        {
            return nullptr;
        }

        GBufferEntry::ModelEntry* entries = allocator->Allocate<GBufferEntry::ModelEntry>(renderables.size());

        uint32_t total_entries = 0;

        for (const ViewableEntry& viewable_entry : renderables)
        {
            const auto* renderable = viewable_entry.component;
        
            // TODO: GameObjects that use the same static Mesh & Material should be instanced.
            // It would be a good idea to add a SetInstancedData method to the ViewContext and macro's
            // in the shaders so that it, for examply, automatically picks the correct instanced model matrix
            // in the Transform helper functions.

            Material*            mat         = nullptr;
            Mesh::VertexPack     vp;
            Shared<VertexBuffer> prim_buffer = nullptr;
            Shared<VertexBuffer> sec_buffer  = nullptr;
            if (viewable_entry.type == ViewableEntryType::Mesh)
            {
                const auto* mesh_renderable = (MeshRenderable*)renderable;
                const auto* mesh = mesh_renderable->GetMesh();

                mat         = mesh_renderable->GetMaterial();
                vp          = mesh->GetVertexPack();
                prim_buffer = vp.primaryBuffer;
                sec_buffer  = vp.secondaryBuffer;
            }
            else
            {
                const auto* mesh_renderable = (SkinnedMeshRenderable*)renderable;
                const auto* mesh = mesh_renderable->GetMesh();

                mat         = mesh_renderable->GetMaterial();
                vp          = mesh->GetVertexPack();
                prim_buffer = view_context->scheduledSkinning ? mesh_renderable->GetSkinnedPrimaryBuffer() : vp.primaryBuffer;
                sec_buffer  = view_context->scheduledSkinning ? mesh_renderable->GetSkinnedSecondaryBuffer() : vp.secondaryBuffer;;
            }

            const Shared<Texture2D>&    albedo      = mat->GetAlbedoTexture();
            const Shared<Texture2D>&    normal      = mat->GetNormalTexture();

            if (!prim_buffer || !prim_buffer->ContentsReady() ||
                (sec_buffer && !sec_buffer->ContentsReady()) ||
                (vp.indexBuffer && !vp.indexBuffer->ContentsReady()) ||
                !albedo || !albedo->ContentsReady() ||
                (normal && !normal->ContentsReady()))
            {
                continue;
            }

            const Transform*     transform = renderable->GetGameObject()->GetComponent<Transform>();
            const Math::Float4x4 model_mat = transform->GetLocalToWorldMatrix();

            GBufferEntry::ModelEntry entry = {};
            entry.vb_primary   = prim_buffer;
            entry.vb_secondary = sec_buffer;
            entry.ib           = vp.indexBuffer;
            entry.albedoTex    = albedo;
            entry.normalTex    = normal;
            entry.modelMatrix  = model_mat;

            entries[total_entries++] = entry;
        }

        GBufferEntry* entry = allocator->Allocate<GBufferEntry>();
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

        in.ri->PushRenderTarget(in.outputRes[0], 0);
        in.ri->PushRenderTarget(in.outputRes[1], 1);
        in.ri->SetDepthStencil(in.outputRes[2]);

        GBufferEntry* entry = (GBufferEntry*)in.entryContext;

        // Set the frame constants
        in.viewContext->SetFrameConstants(GeometryGlobals_FC, in.ri);

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

            Shader::GBufferCB cb = {};
            cb.localToWorldMat = model_entry.modelMatrix;
            cb.hasNormalMap    = model_entry.normalTex != nullptr;

            in.ri->SetConstantShaderData(GeometryGlobals_GBufferCB, &cb, sizeof(Shader::GBufferCB));

            in.ri->SetShaderResourceInput(PsGbuffer_Albedo, model_entry.albedoTex.get());
            if (model_entry.normalTex)
                in.ri->SetShaderResourceInput(PsGbuffer_Normal, model_entry.normalTex.get());

            in.ri->Draw();
        }
    }
}