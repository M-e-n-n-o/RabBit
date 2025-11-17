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
        Frustum     frustum;
    };

    RenderPassConfig CascadedShadowPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const CascadedShadowSettings& s = (const CascadedShadowSettings&)setting;

        return RenderPassConfig(
            {
                // Dependencies
                {},

                // Working textures
                {},

                // Output textures
                {
                    RenderTextureDesc{"CascadedShadowMap", RenderResourceFormat::R32_TYPELESS, 1024, 1024, kRTFlag_CustomSized},
                },

                // Async compute compatible
                false
            });
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
        memset(entries, 0, size);

        uint32_t total_entries = 0;

        for (int i = 0; i < mesh_renderers.size(); ++i)
        {
            const MeshRenderer*     mesh_renderer   = (const MeshRenderer*)mesh_renderers[i];
            const Mesh*             mesh            = mesh_renderer->GetMesh();
            const Mesh::VertexPair& vp              = mesh->GetVertexPair();

            if (!vp.vertexBuffer || !vp.vertexBuffer->ReadyToRender() || 
                (vp.indexBuffer && !vp.indexBuffer->ReadyToRender()))
            {
                continue;
            }

            const Transform* transform = mesh_renderer->GetGameObject()->GetComponent<Transform>();

            CascadedShadowEntry::ModelEntry entry = {};
            entry.vb            = vp.vertexBuffer;
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

        CascadedShadowEntry* entry = new CascadedShadowEntry();
        entry->frustum      = light->CalculateFrustum(*view_context->camera, *view_context->cameraTransform);
        entry->modelEntries = entries;
        entry->entryCount   = total_entries;

        return entry;
    }

    void CascadedShadowPass::Render(RenderPassInput& in)
    {
        in.ri->ClearDepth(in.outputTextures[0], false);

        // Not using a pixel shader
        in.ri->SetVertexShader(VS_Simple3D);

        in.ri->SetBlendMode(BlendMode::None);
        in.ri->SetCullMode(CullMode::Back);
        in.ri->SetDepthMode(DepthMode::PassCloser, true, false);

        in.ri->SetDepthStencil(in.outputTextures[0]);

        CascadedShadowEntry* entry = (CascadedShadowEntry*)in.entryContext;

        // Use the custom frustum
        in.viewContext->SetFrameConstants(in.ri, in.viewContext->viewport, entry->frustum);

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
}