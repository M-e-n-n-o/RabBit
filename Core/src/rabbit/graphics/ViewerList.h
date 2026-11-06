#pragma once

#include "RabBitCommon.h"
#include "entity/Scene.h"

namespace RB::Graphics
{
    class ViewContext;

    enum ViewerListPerspective : uint8_t
    {
        kViewerListPerspective_Camera              = 0,
        kViewerListPerspective_DirectionalLight    = 1,

        kViewerListPerspective_None                = 2,
        kViewerListPerspective_Count               = kViewerListPerspective_None
    };

    enum class ViewableEntryType
    {
        Mesh,           // Entity::MeshRenderable
        SkinnedMesh     // Entity::SkinnedMeshRenderable
    };

    struct ViewableEntry
    {
        ViewableEntryType              type;
        const Entity::ObjectComponent* component;

        // Info per ViewerListType (set to UINT32_MAX by default):
        // - Camera:            -
        // - DirectionalLight:  Which shadow slices actually passed culling
        uint64_t                       extraInfo;
    };

    // Does culling on all ViewableEntryType's based from the specified ViewerListPerspective
    class ViewerList
    {
    public:
        ViewerList(uint8_t perspective);

        void CullEntries(ViewContext* view_context, const Entity::Scene* scene, FrameAllocator* allocator);

        const Span<ViewableEntry> GetViewables() const { return m_GameObjects; }

    private:
        uint8_t               m_Perspective;
        Span<ViewableEntry>   m_GameObjects;
    };
}