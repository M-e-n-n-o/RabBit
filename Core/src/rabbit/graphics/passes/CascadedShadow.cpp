#include "RabBitCommon.h"
#include "CascadedShadow.h"
#include "graphics/Frustum.h"
#include "graphics/View.h"
#include "graphics/RenderInterface.h"

#include "entity/Scene.h"
#include "entity/components/Light.h"
#include "entity/components/Transform.h"
#include "entity/components/Mesh.h"

#include "graphics/shaders/shared/Common.h"
#include "graphics/codeGen/ShaderDefines.h"

using namespace RB::Entity;

namespace RB::Graphics
{
    struct CascadedShadowEntry : public RenderPassEntry
    {
        struct ModelEntry
        {
            Shared<VertexBuffer> vb;
            Shared<IndexBuffer>  ib;
            Math::Float4x4       modelMatrix;
        };

        ModelEntry* modelEntries;
        uint32_t    entryCount;
        Frustum*    frustums;
        uint32_t    totalSlices;
    };

    RenderPassConfig CascadedShadowPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const CascadedShadowSettings& s = (const CascadedShadowSettings&)setting;

        return RenderPassConfig
            {
                // Dependencies
                {},

                // Working textures
                {},

                // Output textures
                {
                    RenderResourceDesc {
                        .name       = "CascadedShadowMap",
                        .format     = RenderResourceFormat::R32_TYPELESS,
                        .type       = RenderResourcePassType::Tex2D,
                        .typeDesc   = { 1024, 1024, m_ShadowSlices },
                        .flags      = kRTFlag_CustomSized | kRTFlag_ClearBeforeGraph,
                        .clearValue = 1.0f
                    }
                },

                // Async compute compatible
                false
            };
    }

    RenderPassEntry* CascadedShadowPass::SubmitEntry(const ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
    {
        const auto& list = scene->GetComponentsWithTypeOf<DirectionalLight>();
        const auto& mesh_renderers = scene->GetComponentsWithTypeOf<MeshRenderer>();

        if (list.empty() || mesh_renderers.empty())
        {
            return nullptr;
        }

        uint32_t size = sizeof(CascadedShadowEntry::ModelEntry) * mesh_renderers.size();
        CascadedShadowEntry::ModelEntry* entries = (CascadedShadowEntry::ModelEntry*)allocator->Allocate(size);

        uint32_t total_entries = 0;

        for (int i = 0; i < mesh_renderers.size(); ++i)
        {
            const MeshRenderer*     mesh_renderer   = (const MeshRenderer*)mesh_renderers[i];
            const Mesh*             mesh            = mesh_renderer->GetMesh();
            const Mesh::VertexPack& vp              = mesh->GetVertexPack();

            if (!vp.primaryBuffer || !vp.primaryBuffer->ReadyToRender() ||
                (vp.indexBuffer && !vp.indexBuffer->ReadyToRender()))
            {
                continue;
            }

            const Transform* transform = mesh_renderer->GetGameObject()->GetComponent<Transform>();

            // TODO: Do frustum culling on each shadow slice

            CascadedShadowEntry::ModelEntry entry = {};
            entry.vb            = vp.primaryBuffer;
            entry.ib            = vp.indexBuffer;
            entry.modelMatrix   = transform->GetLocalToWorldMatrix();

            entries[total_entries] = entry;
            total_entries++;
        }

        if (total_entries == 0)
        {
            return nullptr;
        }

        const auto* light = (DirectionalLight*)list[0];

        uint32_t fsize = sizeof(Frustum) * m_ShadowSlices;
        Frustum* frustums = (Frustum*)allocator->Allocate(fsize);

        for (int i = 0; i < m_ShadowSlices; ++i)
        {
            float split;
            frustums[i] = light->CalculateFrustum(*view_context->camera, *view_context->cameraTransform, i, m_ShadowSlices, &split);
        }

        CascadedShadowEntry* entry = (CascadedShadowEntry*)allocator->Allocate(sizeof(CascadedShadowEntry));
        entry->modelEntries = entries;
        entry->entryCount   = total_entries;
        entry->frustums     = frustums;
        entry->totalSlices  = m_ShadowSlices;

        return entry;
    }

    void CascadedShadowPass::Render(RenderPassInput& in)
    {
        // Not using a pixel shader
        in.ri->SetVertexShader(VS_Simple3D);

        in.ri->SetBlendMode(BlendMode::None);
        in.ri->SetCullMode(CullMode::Back); // should this be front? (breaks on some meshes)
        in.ri->SetDepthMode(DepthMode::PassCloser, true, false);

        Texture2DArray* csm = (Texture2DArray*)in.outputRes[0];

        CascadedShadowEntry* entry = (CascadedShadowEntry*)in.entryContext;

        for (int slice = 0; slice < entry->totalSlices; ++slice)
        {
            RB_PROFILE_GPU_SCOPED(in.ri, "Slice");

            csm->SetFirstArraySlice(slice);
            in.ri->SetDepthStencil(csm);

            // Use the custom frustum
            in.viewContext->SetFrameConstants(in.ri, in.viewContext->viewport, entry->frustums[slice]);

            for (int i = 0; i < entry->entryCount; ++i)
            {
                CascadedShadowEntry::ModelEntry& model_entry = entry->modelEntries[i];

                in.ri->SetVertexBuffer(model_entry.vb.get());

                if (model_entry.ib)
                {
                    in.ri->SetIndexBuffer(model_entry.ib.get());
                }

                in.ri->SetConstantShaderData(kInstanceCB, &model_entry.modelMatrix, sizeof(model_entry.modelMatrix));

                in.ri->Draw();
            }
        }

        // Reset the overwritten properties
        csm->ResetView();
    }
}