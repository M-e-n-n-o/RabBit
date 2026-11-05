#include "RabBitCommon.h"
#include "ViewerList.h"
#include "View.h"

#include "app/FrameAllocator.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Light.h"

using namespace RB::Entity;

namespace RB::Graphics
{
    ViewerList::ViewerList(uint8_t perspective)
        : m_Perspective(perspective)
        , m_GameObjects()
    {
    }

    void ViewerList::CullEntries(ViewContext* view_context, const Scene* scene, FrameAllocator* allocator)
    {
        if (m_Perspective >= kViewerListPerspective_None)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "This ViewerList does not have a valid perspective set");
            return;
        }

        auto mesh_renderables = scene->GetComponentsWithTypeOf<MeshRenderable>(allocator);
        auto skinned_renderables = scene->GetComponentsWithTypeOf<SkinnedMeshRenderable>(allocator);

        if (mesh_renderables.empty() && skinned_renderables.empty())
        {
            return;
        }

        // Camera data
        Math::Float4x4* cam_vp = nullptr;

        // DirectionalLight data
        const Math::Float4x4 cam_world = view_context->cameraTransform.GetLocalToWorldMatrix();
        Span<const DirectionalLight*> dir_lights;
        Span<Math::Float4x4> dir_vps;

        switch (m_Perspective)
        {
        case kViewerListPerspective_Camera:
        {
            cam_vp = allocator->Allocate<Math::Float4x4>();
            *cam_vp = view_context->viewFrustum.GetWorldToViewMatrix() * view_context->viewFrustum.GetViewToClipMatrix();
        }
        break;

        case kViewerListPerspective_DirectionalLight:
        {
            dir_lights = scene->GetComponentsWithTypeOf<DirectionalLight>(allocator);
            
            if (dir_lights.empty())
            {
                return;
            }

            RB_ASSERT(LOGTAG_GRAPHICS, dir_lights.size() == 1, "Only 1 directional light is currently supported");

            const auto* light = dir_lights[0];

            uint32_t slices = light->GetShadowSlices();
            Math::Float4x4* vps = (Math::Float4x4*)allocator->Allocate(sizeof(Math::Float4x4) * slices);
            
            for (int i = 0; i < slices; ++i)
            {
                float split;
                Frustum frustum = light->CalculateFrustum(view_context->camera, view_context->cameraTransform, i, &split);
                vps[i] = frustum.GetWorldToViewMatrix() * frustum.GetViewToClipMatrix();
            }

            dir_vps = Span(vps, slices);
        }
        break;
        
        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "ViewerListType not yet implemented");
            break;
        }

        ViewableEntry* entries = allocator->Allocate<ViewableEntry>(mesh_renderables.size() + skinned_renderables.size());
        uint32_t total_entries = 0;

        auto ProcessRenderables = [&]<typename TRenderable>(const auto& container, ViewableEntryType type)
        {
            for (const ObjectComponent* component : container)
            {
                const auto* renderable = static_cast<const TRenderable*>(component);
                const Mesh* mesh = renderable->GetMesh();

                // Validation check
                if (mesh == nullptr)
                {
                    continue;
                }

                ViewableEntry entry = {};
                entry.component = renderable;
                entry.type      = type;
                entry.extraInfo = UINT32_MAX;

                const Transform*     transform = renderable->GetGameObject()->GetComponent<Transform>();
                const Math::Float4x4 model_mat = transform->GetLocalToWorldMatrix();

                if (mesh->HasValidAABB())
                {
                    // We can do some frustum culling
                    const Math::AABB& aabb       = mesh->GetAABB();
                    const Math::AABB  world_aabb = Math::TransformAABBToWorld(aabb, model_mat);

                    if (m_Perspective == kViewerListPerspective_Camera)
                    {
                        if (!Frustum::IsInFrustum(world_aabb, *cam_vp))
                        {
                            continue;
                        }
                    }
                    else if (m_Perspective == kViewerListPerspective_DirectionalLight)
                    {
                        if constexpr (std::is_same_v<TRenderable, SkinnedMeshRenderable>)
                        {
                            if (Math::Float3::Distance(model_mat.GetPosition(), cam_world.GetPosition()) > renderable->GetSkinningDistance())
                            {
                                continue;
                            }
                        }

                        uint64_t frustum_mask = 0;

                        // Do frustum culling on each shadow slice
                        for (int i = 0; i < dir_vps.size(); ++i)
                        {
                            frustum_mask |= Frustum::IsInFrustum(world_aabb, dir_vps[i]) << i;
                        }

                        if (frustum_mask == 0)
                        {
                            continue;
                        }

                        entry.extraInfo = frustum_mask;
                    }
                }

                entries[total_entries++] = entry;
            }
        };

        ProcessRenderables.operator()<MeshRenderable>(mesh_renderables, ViewableEntryType::Mesh);
        ProcessRenderables.operator()<SkinnedMeshRenderable>(skinned_renderables, ViewableEntryType::SkinnedMesh);

        // TODO: Sort meshes to minimize render state changes when rendering

        m_GameObjects = Span<ViewableEntry>(entries, total_entries);
    }
}