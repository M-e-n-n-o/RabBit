#pragma once

#include "RenderPass.h"

namespace RB::Graphics
{
    class RenderResource;

    using ResourceID = int32_t;

    struct RenderGraphSize
    {
        Math::Float2 size;
    };

    // Holds all the data that is shared between RenderGraph's
    class RenderGraphContext
    {
    public:
        RenderGraphContext() = default;
        ~RenderGraphContext();

        // Gets the base resource (the viewport might be bigger than expected)
        Shared<RenderResource> GetResource(ResourceID id);
        // Gets the alias of the resource with expected viewport size
        Shared<RenderResource> GetResource(ResourceID id, uint32_t graph_id, uint16_t size_id);

        float RequiresClear(ResourceID id);

        uint16_t AddGraphSize(uint32_t graph_id, const RenderGraphSize& size);
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
        // All the resources used by all graphs (base + aliases)
        List<Shared<RenderResource>>        m_Resources;
        // Points to the actual base resources in the m_Resources list
        uint32_t*                           m_BasePointers;
        // Points to aliases in the m_Resources list based on the graph_id & size_id
        UnorderedMap<uint64_t, uint32_t>    m_AliasLookup;
        // Which base resources require a clear before rendering
        List<float>                         m_Clears;

        
        // The rendertexture sizes for each graph
        List<List<RenderGraphSize>>         m_GraphSizes;
        // The scheduled resources for all graphs
        List<RenderResourceDesc>            m_Descriptions;
        // All the resources stored with the graph they are used in
        List<List<ResourceID>>              m_GraphDescriptions;
    };
}