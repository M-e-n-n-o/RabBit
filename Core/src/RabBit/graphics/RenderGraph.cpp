#include "RabBitCommon.h"
#include "RenderGraph.h"

namespace RB::Graphics
{
    // ---------------------------------------------------------------------------
    //                               RenderGraph
    // ---------------------------------------------------------------------------

    // ---------------------------------------------------------------------------
    //                            RenderGraphBuilder
    // ---------------------------------------------------------------------------

    RenderGraphBuilder::RenderGraphBuilder()
        : m_FinalPassType(RenderPassType::None)
    {
        
    }

    RenderGraph RenderGraphBuilder::Build()
    {
        AddLink(RenderPassType::None, RenderPassType::None, 
                        0u,   /*  ->  */   0u,  // Output number 0 of left pass is input number 0 of the right pass
                        1u,   /*  ->  */   3u); // Output number 1 of left pass is input number 3 of the right pass

        /*
            - We first need to figure out the flow of the passes, making sure that passes do not run twice!
            - Then the lifetime of each RenderResource
            - We then need to figure out the flow of these resources between the passes
        */

        if (m_FinalPassType == RenderPassType::None)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "The final pass of the RenderGraph is not properly set yet");
            return {};
        }

        uint64_t processed_mask = 0;
        RenderPassType pass_type = m_FinalPassType;

        List<RenderPass*> passes;
        List<RenderResource*> resources;
        List<RenderGraph::FlowNode> render_flow;

        // Process from back to front
        do
        {
            pass_type = GetNextLeafPass(processed_mask, pass_type);

            auto pass_ptr     = m_Passes.find(pass_type);
            auto settings_ptr = m_PassSettings.find(pass_type);

            if (pass_ptr == m_Passes.end() || settings_ptr == m_PassSettings.end())
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "A pass type of the RenderGraph is not actually added as a pass");
                return {};
            }

            // Mark pass type as processed
            processed_mask |= (1u << (uint64_t) pass_type);

            passes.push_back(pass_ptr->second);

            RenderGraph::FlowNode node = {};
            node.passID = passes.size() - 1;

            // Figure out the lifetime of the resources
            int32_t parameterIDs[MAX_INOUT_RESOURCES_PER_RENDERPASS];
            int32_t workingIDs[MAX_WORKING_RESOURCES_PER_RENDERPASS];
            int32_t outputIDs[MAX_INOUT_RESOURCES_PER_RENDERPASS];

            memset(&parameterIDs[0], -1, _countof(parameterIDs) * sizeof(int32_t));
            memset(&workingIDs[0],   -1, _countof(workingIDs)   * sizeof(int32_t));
            memset(&outputIDs[0],    -1, _countof(outputIDs)    * sizeof(int32_t));

            RenderPassConfig config = pass_ptr->second->GetConfiguration(settings_ptr->second);

            // TODO Figure out the lifetime of resources of this pass.
            // - Look at the parameterID's
            // - Look at the workingID's
            // - Look at the outputID's
            // - Make sure to check if already created resources might also be used for this pass here
            // - Make sure to account for the ContainsHistory flag
            // - The output of the final pass does not need a new resource, that will be provided by the renderer

            // Parameter ID's
            auto connections_ptr = m_Connections.find(pass_type);
            if (connections_ptr != m_Connections.end())
            {
                for (const auto& from_ptr : connections_ptr->second)
                {
                    RenderPassType from_type = from_ptr.first;

                    auto from_pass_ptr = m_Passes.find(from_type);
                    auto from_settings_ptr = m_PassSettings.find(from_type);

                    if (from_pass_ptr == m_Passes.end() || from_settings_ptr == m_PassSettings.end())
                    {
                        RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "A connection's pass type of the RenderGraph is not actually added as a pass");
                        return {};
                    }

                    RenderPassConfig from_config = from_pass_ptr->second->GetConfiguration(from_settings_ptr->second);

                    for (uint32_t i = 0; i < from_ptr.second.size(); i++)
                    {
                        const RenderTextureDesc& desc = from_config.outputTextures[i];

                        // TODO Finish this
                    }
                }
            }

            // Add this pass to the render_flow
            memcpy(node.parameterIDs, parameterIDs, _countof(parameterIDs) * sizeof(int32_t));
            memcpy(node.workingIDs,   workingIDs,   _countof(workingIDs)   * sizeof(int32_t));
            memcpy(node.outputIDs,    parameterIDs, _countof(parameterIDs) * sizeof(int32_t));
            render_flow.push_back(node);

        } while (pass_type != m_FinalPassType);

        // Copy over all the necessary data into an actual RenderGraph
        RenderGraph graph = RenderGraph();
        graph.m_UnorderedPasses = passes;
        graph.m_Resources       = resources;
        graph.m_RenderFlow      = render_flow;

        return graph;
    }

    RenderPassType RenderGraphBuilder::GetNextLeafPass(uint64_t processed_mask, RenderPassType current_type)
    {
        auto itr = m_Connections.find(current_type);

        if (itr == m_Connections.end())
        {
            // This is a leaf pass
            return current_type;
        }
        
        for (const auto& from : itr->second)
        {
            RenderPassType type = from.first;

            if ((processed_mask & (1u << (uint64_t)type)) != 0)
            {
                // This pass is already processed
                continue;
            }

            // See if this pass has any leafs or is the actual leaf
            return GetNextLeafPass(processed_mask, type);
        }

        // All inputs of this pass are processed
        return current_type;
    }
}