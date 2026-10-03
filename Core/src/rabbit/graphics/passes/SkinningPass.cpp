#include "RabBitCommon.h"
#include "SkinningPass.h"

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
    struct SkinningEntry : public RenderPassEntry
    {
        struct ModelEntry
        {
            Shared<VertexBuffer>  positionIn;
            Shared<VertexBuffer>  secondaryIn;
            Shared<VertexBuffer>  boneWeightIn;
            Shared<GenericBuffer> boneMatrixIn;
            Shared<VertexBuffer>  positionOut;
            Shared<VertexBuffer>  secondaryOut;
        };

        ModelEntry*         entries;
        uint32_t            entryCount;
    };

    RenderPassConfig SkinningPass::GetConfiguration(const RenderPassSettings* setting)
    {
        const SkinningSettings* s = (const SkinningSettings*) setting;

        return RenderPassConfig
            {
                // Dependencies
                {},

                // Working textures
                {},

                // Output textures
                {},

                // Async compute compatible
                false
            };
    }

    RenderPassEntry* SkinningPass::SubmitEntry(ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
    {
        auto skinned_renderables = scene->GetComponentsWithTypeOf<SkinnedMeshRenderable>();

        SkinningEntry::ModelEntry* entries = allocator->Allocate<SkinningEntry::ModelEntry>(skinned_renderables.size());

        uint32_t total_entries = 0;

        for (int i = 0; i < skinned_renderables.size(); ++i)
        {
            const SkinnedMeshRenderable* renderable = (const SkinnedMeshRenderable*)skinned_renderables[i];
            const Mesh*                  mesh       = renderable->GetMesh();
            const Mesh::VertexPack&      vp         = mesh->GetVertexPack();

            if (!vp.primaryBuffer || !vp.primaryBuffer->ContentsReady() ||
                !vp.secondaryBuffer || !vp.secondaryBuffer->ContentsReady() ||
                !vp.boneWeightBuffer || !vp.boneWeightBuffer->ContentsReady())
            {
                continue;
            }

            const Transform* transform = renderable->GetGameObject()->GetComponent<Transform>();
            const Math::Float4x4& model_mat = transform->GetLocalToWorldMatrix();

            if (Math::Float3::Distance(model_mat.GetPosition(), view_context->cameraTransform.GetPosition()) > renderable->GetSkinningDistance())
            {
                continue;
            }

            for (int entry_idx = 0; entry_idx < total_entries; entry_idx++)
            {
                if (entries[entry_idx].positionIn == vp.primaryBuffer &&
                    entries[entry_idx].secondaryIn == vp.secondaryBuffer &&
                    entries[entry_idx].boneWeightIn == vp.boneWeightBuffer)
                {
                    // This mesh is used in multiple places, don't skin multiple times
                    continue;
                }
            }

            if (mesh->HasValidAABB())
            {
                // We can do some frustum culling
                const Math::AABB&    aabb       = mesh->GetAABB();
                const Math::AABB     world_aabb = Math::TransformAABBToWorld(aabb, model_mat);
                const Math::Float4x4 vp         = view_context->viewFrustum.GetWorldToViewMatrix() * view_context->viewFrustum.GetViewToClipMatrix();

                if (!Frustum::IsInFrustum(world_aabb, vp))
                {
                    continue;
                }
            }

            SkinningEntry::ModelEntry entry = {};
            entry.positionIn   = vp.primaryBuffer;
            entry.secondaryIn  = vp.secondaryBuffer;
            entry.boneWeightIn = vp.boneWeightBuffer;
            entry.boneMatrixIn = renderable->GetBoneMatrixBuffer();
            entry.positionOut  = renderable->GetSkinnedPrimaryBuffer();
            entry.secondaryOut = renderable->GetSkinnedSecondaryBuffer();

            entries[total_entries] = entry;
            total_entries++;
        }

        SkinningEntry* entry = allocator->Allocate<SkinningEntry>();
        entry->entries      = entries;
        entry->entryCount   = total_entries;

        // Let the other passes know skinning is scheduled
        view_context->scheduledSkinning = true;

        return entry;
    }

    void SkinningPass::Render(RenderPassInput& in)
    {
        in.ri->SetComputeShader(CS_ApplySkinning);

        SkinningEntry* entry = (SkinningEntry*)in.entryContext;

        for (int i = 0; i < entry->entryCount; i++)
        {
            const SkinningEntry::ModelEntry& model_entry = entry->entries[i];

            uint32_t vertex_count = model_entry.positionIn->GetVertexElementCount();
            in.ri->SetConstantShaderData(SkinningGlobals_VertexCount, &vertex_count, sizeof(uint32_t));

            in.ri->SetShaderResourceInput(CsApplySkinning_InPositions,  model_entry.positionIn.get());
            in.ri->SetShaderResourceInput(CsApplySkinning_InSecondary,  model_entry.secondaryIn.get());
            in.ri->SetShaderResourceInput(CsApplySkinning_InSkinVerts,  model_entry.boneWeightIn.get());
            in.ri->SetShaderResourceInput(CsApplySkinning_BoneMatrices, model_entry.boneMatrixIn.get());

            in.ri->SetRandomReadWriteInput(CsApplySkinning_OutPositions, model_entry.positionOut.get());
            in.ri->SetRandomReadWriteInput(CsApplySkinning_OutSecondary, model_entry.secondaryOut.get());

            in.ri->Dispatch(Math::Ceil(vertex_count / 64.0f), 1, 1);
        }
    }
}