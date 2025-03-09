#pragma once

#include "RenderPass.h"
#include "RenderGraphContext.h"

namespace RB::Graphics
{
    class RenderGraphContext;
    class RenderGraphBuilder;

    // ---------------------------------------------------------------------------
    //                               RenderGraph
    // ---------------------------------------------------------------------------

    // Contains all the different RenderPasses that are needed to render a ViewContext
    class RenderGraph
    {
    public:
        virtual ~RenderGraph() = default;

        RenderGraph() = default;

        RenderPassEntry** SubmitEntry(const Entity::Scene* const scene);
        void RunGraph(ViewContext* view_context, RenderPassEntry** entries, RenderInterface* render_interface, RenderGraphContext* graph_context);

    private:
        friend class RenderGraphBuilder;

        struct FlowNode
        {
            uint32_t    passID;
            ResourceID  parameterIDs[MAX_INOUT_RESOURCES_PER_RENDERPASS]; // -1 means not valid
            ResourceID  workingIDs[MAX_WORKING_RESOURCES_PER_RENDERPASS];
            ResourceID  outputIDs[MAX_INOUT_RESOURCES_PER_RENDERPASS];
        };

        uint32_t            m_ID;
        uint32_t            m_FinalOutputResourceID;
        List<RenderPass*>   m_UnorderedPasses;
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

        /*
            Example:
            AddLink(RenderPassType::None, RenderPassType::None, 
                0u,     ->     0u,  // Output number 0 of left pass is input number 0 of the right pass
                1u,     ->     3u); // Output number 1 of left pass is input number 3 of the right pass
        */
        template<class ...ConnectionID>
        RenderGraphBuilder& AddLink(RenderPassType from, RenderPassType to, const ConnectionID&... connection_ids);

        RenderGraph* Build(uint32_t graph_id, RenderGraphContext* context);

    private:
        RenderPassType GetNextLeafPass(uint64_t processed_mask, RenderPassType current_type);

        // TODO Finish this method
        ResourceID GetAlias(const RenderTextureDesc& desc, RenderGraphContext* context, uint32_t graph_id);

        using ResourceConnections = List<uint32_t>;

        // Yes, I know, these types are getting very long and confusing :(
        RenderPassType                                                                  m_FinalPassType;
        uint32_t                                                                        m_FinalResourceId;
        UnorderedMap<RenderPassType, RenderPass*>                                       m_Passes;
        UnorderedMap<RenderPassType, RenderPassSettings>                                m_PassSettings;
        //           To                           From            Resources
        UnorderedMap<RenderPassType, UnorderedMap<RenderPassType, ResourceConnections>> m_Connections;
    };

    template<class Pass>
    inline RenderGraphBuilder& RenderGraphBuilder::AddPass(RenderPassType type, const RenderPassSettings& settings)
    {
        RenderPass* pass = new Pass();

        auto itr = m_Passes.emplace(type, pass);

        if (itr.second)
        {
            m_PassSettings.emplace(type, settings);
        }
        else
        {
            // Already inserted
            delete pass;
        }

        return *this;
    }

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