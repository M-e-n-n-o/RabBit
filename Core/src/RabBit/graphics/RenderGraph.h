#pragma once

#include "RenderPass.h"

namespace RB::Graphics
{
    class RenderGraphBuilder;

    // ---------------------------------------------------------------------------
    //                               RenderGraph
    // ---------------------------------------------------------------------------

    class RenderGraph
    {
    public:
        virtual ~RenderGraph() = default;

        RenderGraph() = default;

        void RunGraph(ViewContext*)
        {
            

            for (int i = 0; i < m_RenderFlow.size(); ++i)
            {
                RenderPass* pass = m_UnorderedPasses[m_RenderFlow[i].passID];

                //pass->Render(dependencies);

                m_RenderFlow[i].parameterIDs == -1
            }
        }

    private:
        friend class RenderGraphBuilder;

        struct FlowNode
        {
            uint32_t passID;
            int32_t  parameterIDs[MAX_INOUT_RESOURCES_PER_RENDERPASS]; // -1 means not valid
            int32_t  workingIDs[MAX_WORKING_RESOURCES_PER_RENDERPASS];
            int32_t  outputIDs[MAX_INOUT_RESOURCES_PER_RENDERPASS];
        };

        RenderPass**        m_UnorderedPasses;
        RenderResource**    m_Resources; // All the resources used by the graph
        List<FlowNode>      m_RenderFlow;
    };

    // ---------------------------------------------------------------------------
    //                            RenderGraphBuilder
    // ---------------------------------------------------------------------------

    // TODO Later in the RabBit editor you should be able to build this graph using a sort of shader graph (see EA SEED's Gigi)
    class RenderGraphBuilder
    {
    public:
        RenderGraphBuilder();

        template<class Pass>
        RenderGraphBuilder& AddPass(RenderPassType type, const RenderPassSettings& settings);

        RenderGraphBuilder& SetFinalPass(RenderPassType type, uint32_t output_id);

        template<class ...ConnectionID>
        RenderGraphBuilder& AddLink(RenderPassType from, RenderPassType to, const ConnectionID&... connection_ids);

        RenderGraph Build();

    private:
        RenderPassType GetNextLeafPass(uint64_t processed_mask, RenderPassType current_type);

        using ResourceConnections = List<uint32_t>;

        // Yes, I know, these types are getting very long and confusing :(
        RenderPassType                                                                  m_FinalPassType;
        UnorderedMap<RenderPassType, RenderPass*>                                       m_Passes;
        UnorderedMap<RenderPassType, RenderPassSettings>                                m_PassSettings;
        //           To                           From            Resources
        UnorderedMap<RenderPassType, UnorderedMap<RenderPassType, ResourceConnections>> m_Connections;
    };

    template<class ...ConnectionID>
    inline RenderGraphBuilder& RenderGraphBuilder::AddLink(RenderPassType from, RenderPassType to, const ConnectionID& ...cs)
    {
        List<uint32_t> connection_ids = { cs... };

        RB_ASSERT(LOGTAG_GRAPHICS, !connection_ids.empty() && (connection_ids.size() % 2 == 0) && connection_ids.size() <= MAX_INOUT_RESOURCES_PER_RENDERPASS, "The RenderGraph connections are not correctly supplied");

        auto to_itr = m_Connections.find(to);

        if (to_itr == m_Connections.end())
        {
            UnorderedMap<RenderPassType, ResourceConnections> list;
            list.emplace(from, connection_ids);

            m_Connections.emplace(to, list);
        }
        else
        {
            auto from_itr = to_itr->second.find(from);

            if (from_itr == to_itr->second.end())
            {
                to_itr->second.emplace(from, connection_ids);
            }
            else
            {
                // Overwrite the previous set of connections
                from_itr->second = connection_ids;
            }
        }

        return *this;
    }
}