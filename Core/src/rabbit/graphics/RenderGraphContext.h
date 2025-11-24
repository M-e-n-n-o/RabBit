#pragma once

#include "RenderPass.h"

namespace RB::Graphics
{
    class RenderResource;

    using ResourceID = int32_t;

    struct RenderGraphSize
    {
        Math::Float2 size;
        Math::Float2 upscaledSize;
        Math::Float2 uiSize;
    };

    // Holds all the data that is shared between RenderGraph's
    class RenderGraphContext
    {
    public:
        RenderGraphContext() = default;
        ~RenderGraphContext();

        Shared<RenderResource> GetResource(ResourceID id);
        float RequiresClear(ResourceID id);

        void AddGraphSize(uint32_t graph_id, const RenderGraphSize& size);
        void DeleteSizes();

        void CreateGraphResources();
        void DeleteGraphResources();
        void DeleteGraphResourceDescriptions();

        // Returns a new ResourceID or returns one that was already
        // created for a different RenderGraph and can be aliased.
        ResourceID ScheduleNewResource(const RenderResourceDesc& desc, uint32_t graph_id);

        RenderResourceDesc GetScheduledResource(ResourceID id);
        List<ResourceID> GetScheduledGraphResources(uint32_t graph_id);

        uint32_t GetTotalCreatedResources() const { return m_Resources.size(); }

    private:
        // All the resources used by all graphs
        List<Shared<RenderResource>> m_Resources;
        List<float>                  m_Clears;
        // Points to the actual resources in the list above
        uint32_t*                    m_ResourcePointers;
        
        // The rendertexture sizes for each graph
        List<List<RenderGraphSize>>  m_GraphSizes;
        // The scheduled resources for all graphs
        List<RenderResourceDesc>     m_Descriptions;
        // All the resources stored with the graph they are used in
        List<List<ResourceID>>       m_GraphDescriptions;
    };
}