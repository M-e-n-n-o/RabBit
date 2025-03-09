#include "RabBitCommon.h"
#include "RenderGraph.h"
#include "RenderGraphContext.h"
#include "View.h"

namespace RB::Graphics
{
    // ---------------------------------------------------------------------------
    //                               RenderGraph
    // ---------------------------------------------------------------------------

    RenderPassEntry** RenderGraph::SubmitEntry(const Entity::Scene* const scene)
    {
        RenderPassEntry** entries = (RenderPassEntry**)ALLOC_HEAP(sizeof(RenderPassEntry*) * m_UnorderedPasses.size());

        bool* submitted = (bool*)ALLOC_STACK(m_UnorderedPasses.size() * sizeof(bool));
        memset(&submitted[0], false, m_UnorderedPasses.size() * sizeof(bool));

        // Gather the entries from all render passes
        for (int i = 0; i < m_RenderFlow.size(); ++i)
        {
            if (submitted[m_RenderFlow[i].passID])
            {
                continue;
            }

            submitted[m_RenderFlow[i].passID] = true;
            entries[i] = m_UnorderedPasses[m_RenderFlow[i].passID]->SubmitEntry(scene);
        }

        return entries;
    }

    void RenderGraph::RunGraph(ViewContext* view_context, RenderPassEntry** entries, RenderInterface* render_interface, RenderGraphContext* graph_context)
    {
        for (int i = 0; i < m_RenderFlow.size(); ++i)
        {
            RenderPass* pass = m_UnorderedPasses[m_RenderFlow[i].passID];

            RenderPassEntry* entry = entries[m_RenderFlow[i].passID];

            // Do not render the pass if it did not submit an entry
            if (entry == nullptr)
            {
                continue;
            }

            RenderResource* parameters[MAX_INOUT_RESOURCES_PER_RENDERPASS] = {};
            RenderResource* intermediates[MAX_WORKING_RESOURCES_PER_RENDERPASS] = {};
            RenderResource* outputs[MAX_INOUT_RESOURCES_PER_RENDERPASS] = {};

            for (int j = 0; j < MAX_INOUT_RESOURCES_PER_RENDERPASS; ++j)
            {
                if (m_RenderFlow[i].parameterIDs[j] != -1)
                    parameters[j] = graph_context->GetResource(m_RenderFlow[i].parameterIDs[j]);
            }

            for (int j = 0; j < MAX_WORKING_RESOURCES_PER_RENDERPASS; ++j)
            {
                if (m_RenderFlow[i].workingIDs[j] != -1)
                    intermediates[j] = graph_context->GetResource(m_RenderFlow[i].workingIDs[j]);
            }

            for (int j = 0; j < MAX_INOUT_RESOURCES_PER_RENDERPASS; ++j)
            {
                if (m_RenderFlow[i].outputIDs[j] != -1)
                    outputs[j] = graph_context->GetResource(m_RenderFlow[i].outputIDs[j]);
            }

            // The final pass uses the output target of the ViewContext
            if (i == m_RenderFlow.size() - 1)
            {
                outputs[m_FinalOutputResourceID] = view_context->finalColorTarget;
            }

            //RB_PROFILE_GPU_SCOPED(command_list.Get(), context->renderPasses[pass_index]->GetName());

            pass->Render(render_interface, view_context, entry, outputs, intermediates, parameters);

            SAFE_DELETE(entry);
        }
        
        SAFE_FREE(entries);
    }

    // ---------------------------------------------------------------------------
    //                            RenderGraphBuilder
    // ---------------------------------------------------------------------------

    RenderGraphBuilder::RenderGraphBuilder()
        : m_FinalPassType(RenderPassType::None)
        , m_FinalResourceId(0)
    {
        
    }

    RenderGraphBuilder& RenderGraphBuilder::SetFinalPass(RenderPassType type, uint32_t output_id)
    {
        m_FinalPassType = type;
        m_FinalResourceId = output_id;

        return *this;
    }

    RenderGraph* RenderGraphBuilder::Build(uint32_t graph_id, RenderGraphContext* context)
    {
        if (m_FinalPassType == RenderPassType::None)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "The final pass of the RenderGraph is not properly set yet");
            return nullptr;
        }

        uint64_t processed_mask = 0;
        RenderPassType pass_type = m_FinalPassType;

        List<RenderPass*> passes;
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
                return nullptr;
            }

            // Mark pass type as processed
            processed_mask |= (1u << (uint64_t) pass_type);

            passes.push_back(pass_ptr->second);

            RenderGraph::FlowNode node = {};
            node.passID = passes.size() - 1;

            // Figure out the lifetime of the resources
            ResourceID parameter_ids[MAX_INOUT_RESOURCES_PER_RENDERPASS];
            ResourceID working_ids[MAX_WORKING_RESOURCES_PER_RENDERPASS];
            ResourceID output_ids[MAX_INOUT_RESOURCES_PER_RENDERPASS];

            memset(&parameter_ids[0], -1, _countof(parameter_ids) * sizeof(ResourceID));
            memset(&working_ids[0],   -1, _countof(working_ids)   * sizeof(ResourceID));
            memset(&output_ids[0],    -1, _countof(output_ids)    * sizeof(ResourceID));

            RenderPassConfig config = pass_ptr->second->GetConfiguration(settings_ptr->second);

            // Parameter ID's
            auto connections_ptr = m_Connections.find(pass_type);
            if (connections_ptr != m_Connections.end())
            {
                uint32_t param_idx = 0;

                for (const auto& from_ptr : connections_ptr->second)
                {
                    RenderPassType from_type = from_ptr.first;

                    auto from_pass_ptr = m_Passes.find(from_type);
                    auto from_settings_ptr = m_PassSettings.find(from_type);

                    if (from_pass_ptr == m_Passes.end() || from_settings_ptr == m_PassSettings.end())
                    {
                        RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "A connection's pass type of the RenderGraph is not actually added as a pass");
                        return nullptr;
                    }

                    RenderPassConfig from_config = from_pass_ptr->second->GetConfiguration(from_settings_ptr->second);

                    for (uint32_t i = 0; i < from_ptr.second.size(); i++)
                    {
                        if (param_idx >= MAX_INOUT_RESOURCES_PER_RENDERPASS)
                        {
                            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has too many input resources, we currently only support %d", (uint32_t)pass_type, MAX_INOUT_RESOURCES_PER_RENDERPASS);
                            return nullptr;
                        }

                        const RenderTextureDesc& desc = from_config.outputTextures[i];

                        ResourceID id = GetAlias(desc, context, graph_id);

                        if (id == -1)
                        {
                            // No alias found
                            parameter_ids[param_idx] = context->ScheduleNewResource(desc, graph_id);
                        }
                        else
                        {
                            // Found an alias
                            parameter_ids[param_idx] = id;

                            // Make sure to combine the resource flags of the already scheduled resource with the current resource' flags
                            context->CombineScheduledResourceFlags(id, desc.flags);
                        }

                        param_idx++;
                    }
                }
            }

            // Working ID's
            if (config.totalWorkingTextures > MAX_WORKING_RESOURCES_PER_RENDERPASS)
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has too many working textures, we currently only support %d", (uint32_t)pass_type, MAX_WORKING_RESOURCES_PER_RENDERPASS);
                return nullptr;
            }

            for (uint32_t i = 0; i < config.totalWorkingTextures; ++i)
            {
                const RenderTextureDesc& desc = config.workingTextures[i];

                // No need to check the lifetime of working textures, they are only used by this pass
                working_ids[i] = context->ScheduleNewResource(desc, graph_id);
            }

            // Output ID's
            if (config.totalOutputTextures > MAX_INOUT_RESOURCES_PER_RENDERPASS)
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has too many output textures, we currently only support %d", (uint32_t)pass_type, MAX_INOUT_RESOURCES_PER_RENDERPASS);
                return nullptr;
            }

            for (uint32_t i = 0; i < config.totalOutputTextures; ++i)
            {
                if (pass_type == m_FinalPassType && i == m_FinalResourceId)
                {
                    // The final pass doesn't need a dedicated output texture for this one, it will use the one of the ViewContext
                    continue;
                }

                const RenderTextureDesc& desc = config.outputTextures[i];

                ResourceID id = GetAlias(desc, context, graph_id);

                if (id == -1)
                {
                    // No alias found
                    output_ids[i] = context->ScheduleNewResource(desc, graph_id);
                }
                else
                {
                    // Found an alias
                    output_ids[i] = id;

                    // Make sure to combine the resource flags of the already scheduled resource with the current resource' flags
                    context->CombineScheduledResourceFlags(id, desc.flags);
                }
            }

            // Add this pass to the render_flow
            memcpy(node.parameterIDs, parameter_ids, _countof(parameter_ids) * sizeof(ResourceID));
            memcpy(node.workingIDs,   working_ids,   _countof(working_ids)   * sizeof(ResourceID));
            memcpy(node.outputIDs,    output_ids,    _countof(output_ids)    * sizeof(ResourceID));
            render_flow.push_back(node);

        } while (pass_type != m_FinalPassType);

        // Copy over all the necessary data into an actual RenderGraph
        RenderGraph* graph = new RenderGraph();
        graph->m_ID                      = graph_id;
        graph->m_FinalOutputResourceID   = m_FinalResourceId;
        graph->m_UnorderedPasses         = passes;
        graph->m_RenderFlow              = render_flow;

        // Remove all the passes that were not needed in the end, if any
        for (auto itr = m_Passes.begin(); itr != m_Passes.end(); ++itr)
        {
            if (std::find(passes.begin(), passes.end(), itr->second) == passes.end())
            {
                RB_LOG(LOGTAG_GRAPHICS, "Found an unused RenderPass in the graph, %d", (uint32_t)itr->first);
                delete itr->second;
            }
        }

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
    
    ResourceID RenderGraphBuilder::GetAlias(const RenderTextureDesc& desc, RenderGraphContext* context, uint32_t graph_id)
    {
        ResourceID id = -1;

        List<ResourceID>& resources = context->GetScheduledGraphResources(graph_id);

        for (const ResourceID& other_id : resources)
        {
            const RenderTextureDesc& other = context->GetScheduledResource(other_id);

            if (!desc.IsAliasableWith(other))
                continue;

            // TODO Future optimization, figure out if they overlap in lifetime and use as alias
        }

        return id;
    }
}