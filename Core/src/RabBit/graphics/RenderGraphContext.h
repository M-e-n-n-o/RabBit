#pragma once

#include "RenderPass.h"

namespace RB::Graphics
{
    class RenderResource;

    using ResourceID = int32_t;

    // Holds all the data that is shared between RenderGraph's
    class RenderGraphContext
    {
    public:
        RenderGraphContext() = default;
        ~RenderGraphContext();

        RenderResource* GetResource(ResourceID id);

        void CreateGraphResources(Math::Float2 size, Math::Float2 upscaled_size, Math::Float2 ui_size);
        void DeleteGraphResources();

        // !!! TODO Make sure to combine the ResourceFlags when we found an alias !!!
        // Returns a new ResourceID or returns one that was already
        // created for a different RenderGraph and can be aliased.
        ResourceID ScheduleNewResource(const RenderTextureDesc& desc, uint32_t graph_id);

        // Updates the RenderResource::flags, used when resources are aliasable but have different flags
        void CombineScheduledResourceFlags(ResourceID id, uint32_t new_flags);

        RenderTextureDesc GetScheduledResource(ResourceID id);
        List<ResourceID> GetScheduledGraphResources(uint32_t graph_id);

    private:
        // All the resources used by all graphs
        List<RenderResource*> m_Resources;

        // The scheduled resources for all graphs
        List<RenderTextureDesc> m_Descriptions;
        // All the resources stored with the graph they are used in
        List<List<ResourceID>> m_GraphDescriptions;
    };
}