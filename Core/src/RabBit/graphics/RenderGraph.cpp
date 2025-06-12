#include "RabBitCommon.h"
#include "RenderGraph.h"
#include "RenderGraphContext.h"
#include "RenderInterface.h"
#include "View.h"

namespace RB::Graphics
{
    // ---------------------------------------------------------------------------
    //                               RenderGraph
    // ---------------------------------------------------------------------------

    RenderGraph::~RenderGraph()
    {
        for (const auto& pass : m_UnorderedPasses) 
        {
            // Delete the renderpasses
            delete pass.second;
        }
    }

    RenderPassEntry** RenderGraph::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene)
    {
        size_t size = sizeof(RenderPassEntry*) * m_RenderFlow.size();
        RenderPassEntry** entries = (RenderPassEntry**)ALLOC_HEAP(size);
        memset(&entries[0], 0, size);

        bool* submitted = (bool*)ALLOC_STACK(((uint32_t)RenderPassType::Count) * sizeof(bool));
        memset(&submitted[0], false, ((uint32_t)RenderPassType::Count) * sizeof(bool));

        // Gather the entries from all render passes
        for (int idx = 0; idx < m_RenderFlow.size(); ++idx)
        {
            uint32_t id = m_RenderFlow[idx].passID;

            if (submitted[id])
            {
                // We can not submit an entry to the same pass multiple times
                continue;
            }

            submitted[id] = true;
            entries[idx] = m_UnorderedPasses[id]->SubmitEntry(view_context, scene);
        }

        return entries;
    }

    void RenderGraph::RunGraph(ViewContext* view_context, RenderPassEntry** entries, RenderInterface* render_interface, RenderGraphContext* graph_context)
    {
        // First clear the necessary resources
        {
            RB_PROFILE_GPU_SCOPED(render_interface, "Clear");

            const List<ResourceID>& all_resources = graph_context->GetScheduledGraphResources(m_ID);
            for (const ResourceID& id : all_resources)
            {
                if (graph_context->RequiresClear(id))
                {
                    render_interface->Clear(graph_context->GetResource(id));
                }
            }

            render_interface->FlushAllPending();
        }

        // Then actually execute the graph
        for (int i = 0; i < m_RenderFlow.size(); ++i)
        {
            RenderPass*      pass  = m_UnorderedPasses[m_RenderFlow[i].passID];
            RenderPassEntry* entry = entries[i];

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
                else
                    parameters[j] = nullptr;
            }

            for (int j = 0; j < MAX_WORKING_RESOURCES_PER_RENDERPASS; ++j)
            {
                if (m_RenderFlow[i].workingIDs[j] != -1)
                    intermediates[j] = graph_context->GetResource(m_RenderFlow[i].workingIDs[j]);
                else
                    intermediates[j] = nullptr;
            }

            for (int j = 0; j < MAX_INOUT_RESOURCES_PER_RENDERPASS; ++j)
            {
                if (m_RenderFlow[i].outputIDs[j] != -1)
                    outputs[j] = graph_context->GetResource(m_RenderFlow[i].outputIDs[j]);
                else
                    outputs[j] = nullptr;
            }

            // The final pass uses the output target of the ViewContext
            if (i == m_RenderFlow.size() - 1)
            {
                outputs[m_FinalOutputResourceID] = view_context->finalColorTarget;
            }

            RB_PROFILE_GPU_SCOPED(render_interface, pass->GetName());

            // Clear the render state before every pass
            render_interface->InvalidateState(false);

            RenderPassInput input;
            input.viewContext           = view_context;
            input.renderInterface       = render_interface;
            input.entryContext          = entry;
            input.dependencyTextures    = parameters;
            input.workingTextures       = intermediates;
            input.outputTextures        = outputs;

            pass->Render(input);
        }
    }

    void RenderGraph::DestroyEntries(RenderPassEntry** entries)
    {
        for (int i = 0; i < m_RenderFlow.size(); ++i)
        {
            SAFE_DELETE(entries[i]);
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
        RenderPassType pass_type = RenderPassType::None;

        UnorderedMap<uint32_t, RenderPass*> passes;
        List<RenderGraph::FlowNode> render_flow;

        // Process from back to front
        do
        {
            pass_type = GetNextLeafPass(processed_mask, m_FinalPassType);

            auto pass_ptr     = m_Passes.find(pass_type);
            auto settings_ptr = m_PassSettings.find(pass_type);

            if (pass_ptr == m_Passes.end() || settings_ptr == m_PassSettings.end())
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "A pass type of the RenderGraph is not actually added as a pass");
                return nullptr;
            }

            // Mark pass type as processed
            processed_mask |= (1u << (uint64_t) pass_type);

            passes.emplace((uint32_t)pass_ptr->first, pass_ptr->second);

            RenderGraph::FlowNode node = {};
            node.passID = (uint32_t)pass_ptr->first;

            // Figure out the lifetime of the resources
            ResourceID parameter_ids[MAX_INOUT_RESOURCES_PER_RENDERPASS];
            ResourceID working_ids[MAX_WORKING_RESOURCES_PER_RENDERPASS];
            ResourceID output_ids[MAX_INOUT_RESOURCES_PER_RENDERPASS];

            memset(&parameter_ids[0], -1, _countof(parameter_ids) * sizeof(ResourceID));
            memset(&working_ids[0],   -1, _countof(working_ids)   * sizeof(ResourceID));
            memset(&output_ids[0],    -1, _countof(output_ids)    * sizeof(ResourceID));

            RenderPassConfig config = pass_ptr->second->GetConfiguration(settings_ptr->second);

            auto GetAlias = [&](const RenderTextureDesc& desc, bool check_lifetime) -> ResourceID
            {
                ResourceID id = -1;

                // Get the already scheduled resources for this graph
                List<ResourceID>& resources = context->GetScheduledGraphResources(graph_id);

                for (const ResourceID& other_id : resources)
                {
                    const RenderTextureDesc& other = context->GetScheduledResource(other_id);

                    if (!desc.IsAliasableWith(other))
                        continue;

                    // Check to not alias with resurces from the current pass
                    {
                        bool match = false;

                        for (int i = 0; i < _countof(parameter_ids) && !match; ++i)
                        {
                            if (parameter_ids[i] == other_id)
                                match = true;
                        }
                        for (int i = 0; i < _countof(working_ids) && !match; ++i)
                        {
                            if (working_ids[i] == other_id)
                                match = true;
                        }
                        for (int i = 0; i < _countof(output_ids) && !match; ++i)
                        {
                            if (output_ids[i] == other_id)
                                match = true;
                        }

                        if (match)
                            continue;
                    }

                    if (!check_lifetime)
                    {
                        id = other_id;
                        break;
                    }

                    // TODO Future optimization, figure out if they don't overlap in lifetime and use as alias
                }

                return id;
            };

            // Parameter ID's & linked inouts
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
                        return nullptr;
                    }

                    RenderPassConfig from_config = from_pass_ptr->second->GetConfiguration(from_settings_ptr->second);

                    for (uint32_t i = 0; i < from_ptr.second.size(); i += 2)
                    {
                        uint32_t from_res_idx = from_ptr.second[i];
                        uint32_t to_res_idx = from_ptr.second[i + 1];

                        if (to_res_idx >= config.totalDependencies)
                        {
                            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has more linked input resources from RenderPass %d than it has inputs", (uint32_t)pass_type, (uint32_t)from_pass_ptr->first);
                            return nullptr;
                        }

                        if (to_res_idx >= MAX_INOUT_RESOURCES_PER_RENDERPASS)
                        {
                            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has too many input resources, we currently only support %d", (uint32_t)pass_type, MAX_INOUT_RESOURCES_PER_RENDERPASS);
                            return nullptr;
                        }

                        const RenderTextureDesc& from_desc = from_config.outputTextures[from_res_idx];

                        if (IsDepthFormat(from_desc.format) != config.dependencies[to_res_idx].depthFormat)
                        {
                            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has resource %d linked from RenderPass %d which have incompatible formats", (uint32_t)pass_type, to_res_idx, (uint32_t)from_pass_ptr->first);
                            return nullptr;
                        }
                        
                        int32_t linked_out_idx = config.dependencies[to_res_idx].outputTextureIndex;
                        if (linked_out_idx >= 0)
                        {
                            // This parameter is also an output

                            if (IsDepthFormat(config.outputTextures[linked_out_idx].format) != config.dependencies[to_res_idx].depthFormat)
                            {
                                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has resource %d that is an in- and output that have incompatible formats", (uint32_t)pass_type, to_res_idx);
                                return nullptr;
                            }

                            if (from_pass_ptr->first == m_FinalPassType && from_res_idx == m_FinalResourceId)
                            {
                                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Its currently not possible for the final output resource of the rendergraph to be an inout resource", (uint32_t)pass_type, to_res_idx);
                                return nullptr;
                            }

                            // The pass will use the output parameter to access this input
                            parameter_ids[to_res_idx] = -1;

                            const RenderTextureDesc& desc = config.outputTextures[linked_out_idx];

                            // Create the linked inout texture
                            ResourceID id = GetAlias(desc, true);

                            if (id == -1)
                            {
                                // No alias found
                                output_ids[linked_out_idx] = context->ScheduleNewResource(desc, graph_id);
                            }
                            else
                            {
                                // Found an alias
                                output_ids[linked_out_idx] = id;
                                // Make sure to combine the resource flags of the already scheduled resource with the current resource' flags
                                context->GetScheduledResource(id).CombineFlags(desc.flags);
                            }
                        }
                        else
                        {
                            // This is just a regular input

                            ResourceID from_res = -1;

                            // Find the scheduled resources of the from pass
                            for (const auto& node : render_flow)
                            {
                                if (node.passID == (uint32_t)from_pass_ptr->first)
                                {
                                    from_res = node.outputIDs[from_res_idx];
                                    break;
                                }
                            }

                            if (from_res == -1)
                            {
                                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Something is wrong in the logic of the RenderGraphBuilder, the output RenderPass %d should have been processed before RenderPass %d", (uint32_t)from_pass_ptr->first, (uint32_t)pass_type);
                                return nullptr;
                            }

                            parameter_ids[to_res_idx] = from_res;
                        }
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

                // No need to check the full lifetime of working textures, they are only used by this pass
                ResourceID id = GetAlias(desc, false);

                if (id == -1)
                {
                    // No alias found
                    working_ids[i] = context->ScheduleNewResource(desc, graph_id);
                }
                else
                {
                    // Found an alias
                    working_ids[i] = id;
                    // Make sure to combine the resource flags of the already scheduled resource with the current resource' flags
                    context->GetScheduledResource(id).CombineFlags(desc.flags);
                }

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
                    if (config.outputTextures[i].width != kRTSize_Full || config.outputTextures[i].height != kRTSize_Full)
                    {
                        RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "The final resource (%d) of the final RenderPass (%d) should have a width and height of kRTSize_Full", m_FinalResourceId, (uint32_t)pass_type);
                        return nullptr;
                    }

                    // The final pass doesn't need a dedicated output texture for this one, it will use the one of the ViewContext
                    continue;
                }

                if (output_ids[i] != -1)
                {
                    // This output texture is also an input texture linked to a different pass, so we don't have to create a new texture
                    continue;
                }

                const RenderTextureDesc& desc = config.outputTextures[i];

                ResourceID id = GetAlias(desc, true);

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
                    context->GetScheduledResource(id).CombineFlags(desc.flags);
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
            bool found = false;
            for (auto other_itr = passes.begin(); other_itr != passes.end(); other_itr++)
            {
                if ((uint32_t)itr->first == other_itr->first)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                RB_LOG(LOGTAG_GRAPHICS, "Detected an unused RenderPass in the graph, %d", (uint32_t)itr->first);
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
}