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
#include "codeGen/ShaderDefines.h"

using namespace RB::Entity;

namespace RB::Graphics
{
    struct CascadedShadowEntry : public RenderPassEntry
    {
        struct ModelEntry
        {
            uint32_t             frustumMask;
            Shared<VertexBuffer> vb;
            Shared<IndexBuffer>  ib;
            Math::Float4x4       modelMatrix;
        };

        ModelEntry* modelEntries;
        uint32_t    entryCount;
        Frustum*    frustums;
        uint32_t    totalSlices;
    };

    RenderPassConfig CascadedShadowPass::GetConfiguration(const RenderPassSettings* setting)
    {
        const CascadedShadowSettings* s = (const CascadedShadowSettings*)setting;

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
                        .typeDesc   = { 2048, 2048, Shader::MAX_NUM_CASCADES },
                        .flags      = kRTFlag_CustomSized | kRTFlag_ClearBeforeGraph,
                        .clearValue = 1.0f
                    }
                },

                // Async compute compatible
                false,

                kViewerListPerspective_DirectionalLight
            };
    }

    RenderPassEntry* CascadedShadowPass::SubmitEntry(ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
    {
        const auto& lights = scene->GetComponentsWithTypeOf<DirectionalLight>(allocator);
        const auto& renderables = view_context->viewerLists[kViewerListPerspective_DirectionalLight]->GetViewables();

        if (lights.empty() || renderables.empty())
        {
            return nullptr;
        }

        CascadedShadowEntry::ModelEntry* entries = allocator->Allocate<CascadedShadowEntry::ModelEntry>(renderables.size());

        const auto* light = lights[0];
        RB_ASSERT(LOGTAG_GRAPHICS, lights.size() == 1, "Only 1 directional light is currently supported");
        
        uint32_t total_slices = light->GetShadowSlices();
        if (total_slices > Shader::MAX_NUM_CASCADES)
        {
            total_slices = Shader::MAX_NUM_CASCADES;
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Shadow slices of directional light is larger than CascadedShadowPass allows (you may see shadows clipping)");
        }

        uint32_t fsize = sizeof(Frustum) * total_slices;
        Frustum* frustums = (Frustum*)allocator->Allocate(fsize);

        List<Math::Float4x4> precomputed_vps(total_slices);
        for (int i = 0; i < total_slices; ++i)
        {
            float split;
            frustums[i] = light->CalculateFrustum(view_context->camera, view_context->cameraTransform, i, &split, total_slices);

            precomputed_vps[i] = frustums[i].GetWorldToViewMatrix() * frustums[i].GetViewToClipMatrix();
        }

        uint32_t total_entries = 0;

        for (const ViewableEntry& viewable_entry : renderables)
        {
            const auto* renderable = viewable_entry.component;

            Mesh::VertexPack vp;
            Shared<VertexBuffer> prim = nullptr;
            if (viewable_entry.type == ViewableEntryType::Mesh)
            {
                const auto* mesh = ((MeshRenderable*)renderable)->GetMesh();
                vp = mesh->GetVertexPack();
                prim = vp.primaryBuffer;
            }
            else
            {
                const auto* mesh_renderable = (SkinnedMeshRenderable*)renderable;
                const auto* mesh = mesh_renderable->GetMesh();
                vp = mesh->GetVertexPack();
                prim = view_context->scheduledSkinning ? mesh_renderable->GetSkinnedPrimaryBuffer() : vp.primaryBuffer;;
            }


            if (!prim || !prim->ContentsReady() ||
                (vp.indexBuffer && !vp.indexBuffer->ContentsReady()))
            {
                continue;
            }

            const Transform* transform = renderable->GetGameObject()->GetComponent<Transform>();
            const Math::Float4x4 model_mat = transform->GetLocalToWorldMatrix();

            CascadedShadowEntry::ModelEntry entry = {};
            entry.frustumMask       = viewable_entry.extraInfo;
            entry.vb                = prim;
            entry.ib                = vp.indexBuffer;
            entry.modelMatrix       = model_mat;
            entries[total_entries]  = entry;
            total_entries++;
        }

        if (total_entries == 0)
        {
            return nullptr;
        }

        CascadedShadowEntry* entry = allocator->Allocate<CascadedShadowEntry>();
        entry->modelEntries = entries;
        entry->entryCount   = total_entries;
        entry->frustums     = frustums;
        entry->totalSlices  = total_slices;

        return entry;
    }

    void CascadedShadowPass::Render(RenderPassInput& in)
    {
        // Not using a pixel shader
        in.ri->SetVertexShader(Shader::VS_Simple3D);

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
            in.viewContext->SetFrameConstants(Shader::SimpleGlobals_FC, in.ri, in.viewContext->viewport, entry->frustums[slice]);

            for (int i = 0; i < entry->entryCount; ++i)
            {
                CascadedShadowEntry::ModelEntry& model_entry = entry->modelEntries[i];

                if ((model_entry.frustumMask & (1U << slice)) == 0)
                {
                    continue;
                }

                in.ri->SetVertexBuffer(model_entry.vb.get());

                if (model_entry.ib)
                {
                    in.ri->SetIndexBuffer(model_entry.ib.get());
                }

                in.ri->SetConstantShaderData(Shader::SimpleGlobals_CustomMatrix, &model_entry.modelMatrix, sizeof(model_entry.modelMatrix));

                in.ri->Draw();
            }
        }

        // Reset the overwritten properties
        csm->ResetView();
    }
}