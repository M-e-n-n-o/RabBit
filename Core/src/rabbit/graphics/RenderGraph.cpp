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

    #define VIEWCONTEXT_OUTPUT_ID INT32_MAX

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

    void RenderGraph::RunGraph(ViewContext* view_context, FrameAllocator* allocator, RenderPassEntry** entries, RenderInterface* render_interface, RenderGraphContext* graph_context)
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
                if (m_RenderFlow[i].parameterIDs[j] == VIEWCONTEXT_OUTPUT_ID)
                    parameters[j] = view_context->finalColorTarget;
                else if (m_RenderFlow[i].parameterIDs[j] != -1)
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
                if (m_RenderFlow[i].outputIDs[j] == VIEWCONTEXT_OUTPUT_ID)
                    outputs[j] = view_context->finalColorTarget;
                else if (m_RenderFlow[i].outputIDs[j] != -1)
                    outputs[j] = graph_context->GetResource(m_RenderFlow[i].outputIDs[j]);
                else
                    outputs[j] = nullptr;
            }

            RB_PROFILE_GPU_SCOPED(render_interface, pass->GetName());

            // Clear the render state before every pass
            render_interface->InvalidateState(false);

            RenderPassInput input;
            input.viewContext           = view_context;
            input.frameAllocator        = allocator;
            input.ri                    = render_interface;
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


        // Collect pass configs
        struct GraphInfo
        {
            UnorderedMap<RenderPassType, RenderPassConfig> configs;
            decltype(m_Connections) connections;
        } info;

        info.connections = m_Connections;

        for (auto& p : m_Passes)
        {
            auto settings_itr = m_PassSettings.find(p.first);
            if (settings_itr == m_PassSettings.end())
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "RenderPass %d has no settings", (uint32_t)p.first);
                return nullptr;
            }
            info.configs[p.first] = p.second->GetConfiguration(settings_itr->second);
        }


        // Determine pass order (topological sort, producers first, depth first search)
        List<RenderPassType> ordered_passes;
        UnorderedSet<uint32_t> visited;
        std::function<void(RenderPassType)> DFS = [&](RenderPassType pass)
        {
            if (visited.find((uint32_t)pass) != visited.end())
                return;
            visited.insert((uint32_t)pass);

            auto it = info.connections.find(pass);
            if (it != info.connections.end())
            {
                for (auto& from : it->second)
                    DFS(from.first);
            }

            ordered_passes.push_back(pass); // Add after producers
        };
        DFS(m_FinalPassType);


        // Track outputs that must use viewcontext resource
        UnorderedMap<RenderPassType, uint32_t> pending_view_outputs;
        auto MarkViewOutput = [&](RenderPassType p, uint32_t idx)
        {
            pending_view_outputs[p] |= (1u << idx);
        };
        auto IsViewOutput = [&](RenderPassType p, uint32_t idx) -> bool
        {
            auto it = pending_view_outputs.find(p);
            return it != pending_view_outputs.end() && ((it->second & (1u << idx)) != 0);
        };
        

        // Map outputs to linked consumers
        UnorderedMap<RenderPassType, UnorderedMap<uint32_t, List<std::pair<RenderPassType, uint32_t>>>> downstream_map;
        for (auto& conn : info.connections)
        {
            RenderPassType consumer = conn.first;
            for (auto& pair : conn.second)
            {
                RenderPassType producer = pair.first;
                const List<uint32_t>& flat = pair.second;
                for (size_t i = 0; i + 1 < flat.size(); i += 2)
                {
                    downstream_map[producer][flat[i]].push_back({ consumer, flat[i + 1] });
                }
            }
        }


        // Recursive function to propagate viewcontext upstream
        std::function<void(RenderPassType, uint32_t)> PropagateViewOutput = [&](RenderPassType pass, uint32_t out_idx)
        {
            if (IsViewOutput(pass, out_idx))
                return; // Already marked

            MarkViewOutput(pass, out_idx);

            // Check producers of this output (linked in-outs)
            auto itConn = m_Connections.find(pass);
            if (itConn == m_Connections.end()) 
                return;

            const RenderPassConfig& config = info.configs[pass];

            for (auto& from_pair : itConn->second)
            {
                RenderPassType producer = from_pair.first;
                const List<uint32_t>& flat = from_pair.second;

                for (size_t i = 0; i + 1 < flat.size(); i += 2)
                {
                    uint32_t from_res_idx = flat[i];
                    uint32_t to_res_idx = flat[i + 1];

                    // Only propagate if this input is linked in-out (consumer output index == out_idx)
                    int32_t linked_out_idx = config.dependencies[to_res_idx].outputTextureIndex;
                    if (linked_out_idx == (int32_t)out_idx)
                    {
                        PropagateViewOutput(producer, from_res_idx);
                    }
                }
            }
        };


        // Initial propagation from final pass
        const RenderPassConfig& final_cfg = info.configs[m_FinalPassType];
        for (uint32_t i = 0; i < _countof(final_cfg.outputTextures); ++i)
        {
            if (i == (uint32_t)m_FinalResourceId)
            {
                PropagateViewOutput(m_FinalPassType, i);
            }
        }


        // Build render flow
        List<RenderGraph::FlowNode> render_flow;
        UnorderedMap<uint32_t, RenderPass*> used_passes;

        auto GetAlias = [&](const RenderTextureDesc& desc, ResourceID* parameter_ids, ResourceID* working_ids, ResourceID* output_ids, bool check_lifetime) -> ResourceID
        {
            ResourceID id = -1;
            const List<ResourceID>& resources = context->GetScheduledGraphResources(graph_id);

            for (ResourceID other_id : resources)
            {
                const RenderTextureDesc& other = context->GetScheduledResource(other_id);

                if (!desc.IsAliasableWith(other)) 
                    continue;

                // Don't alias with current pass
                bool match = false;
                for (int i = 0; i < MAX_INOUT_RESOURCES_PER_RENDERPASS && !match; ++i)   if (parameter_ids[i] == other_id) match = true;
                for (int i = 0; i < MAX_WORKING_RESOURCES_PER_RENDERPASS && !match; ++i) if (working_ids[i] == other_id) match = true;
                for (int i = 0; i < MAX_INOUT_RESOURCES_PER_RENDERPASS && !match; ++i)   if (output_ids[i] == other_id) match = true;
                if (match) continue;

                if (!check_lifetime) 
                { 
                    id = other_id; 
                    break; 
                }

                // TODO Future optimization, figure out if they don't overlap in lifetime and use as alias
            }

            return id;
        };

        for (RenderPassType pass_type : ordered_passes)
        {
            auto pass_ptr = m_Passes.find(pass_type);
            auto settings_ptr = m_PassSettings.find(pass_type);
            RB_ASSERT(LOGTAG_GRAPHICS, pass_ptr != m_Passes.end() && settings_ptr != m_PassSettings.end(), "Missing pass");

            used_passes[(uint32_t)pass_ptr->first] = pass_ptr->second;

            RenderGraph::FlowNode node = {};
            node.passID = (uint32_t)pass_ptr->first;

            ResourceID parameter_ids[MAX_INOUT_RESOURCES_PER_RENDERPASS];
            ResourceID working_ids[MAX_WORKING_RESOURCES_PER_RENDERPASS];
            ResourceID output_ids[MAX_INOUT_RESOURCES_PER_RENDERPASS];

            memset(parameter_ids, -1, sizeof(parameter_ids));
            memset(working_ids, -1, sizeof(working_ids));
            memset(output_ids, -1, sizeof(output_ids));

            const RenderPassConfig& config = info.configs[pass_type];

            // Create outputs
            for (uint32_t i = 0; i < _countof(config.outputTextures); ++i)
            {
                if (config.outputTextures[i].flags == UINT32_MAX)
                    break;

                if (IsViewOutput(pass_type, i))
                {
                    output_ids[i] = VIEWCONTEXT_OUTPUT_ID;
                    continue;
                }

                ResourceID alias = GetAlias(config.outputTextures[i], parameter_ids, working_ids, output_ids, true);
                if (alias == -1)
                    output_ids[i] = context->ScheduleNewResource(config.outputTextures[i], graph_id);
                else
                {
                    output_ids[i] = alias;
                    context->GetScheduledResource(alias).CombineFlags(config.outputTextures[i].flags);
                }
            }

            // Assign parameters
            auto conn_it = m_Connections.find(pass_type);
            if (conn_it != m_Connections.end())
            {
                for (auto& from_pair : conn_it->second)
                {
                    RenderPassType producer = from_pair.first;
                    const List<uint32_t>& flat = from_pair.second;

                    for (size_t j = 0; j + 1 < flat.size(); j += 2)
                    {
                        uint32_t from_idx = flat[j];
                        uint32_t to_idx = flat[j + 1];

                        int32_t linked_out_idx = config.dependencies[to_idx].outputTextureIndex;
                        if (linked_out_idx >= 0)
                        {
                            parameter_ids[to_idx] = output_ids[linked_out_idx];
                        }
                        else
                        {
                            int producerNode = -1;
                            for (size_t k = 0; k < render_flow.size(); ++k)
                            {
                                if (render_flow[k].passID == (uint32_t)producer)
                                {
                                    producerNode = (int)k;
                                    break;
                                }
                            }
                            RB_ASSERT(LOGTAG_GRAPHICS, producerNode != -1, "Producer %d not added yet", from_idx);
                            parameter_ids[to_idx] = render_flow[producerNode].outputIDs[from_idx];
                        }
                    }
                }
            }

            // Working textures
            for (uint32_t i = 0; i < _countof(config.workingTextures); ++i)
            {
                if (config.workingTextures[i].flags == UINT32_MAX)
                    break;

                ResourceID alias = GetAlias(config.workingTextures[i], parameter_ids, working_ids, output_ids, false);
                if (alias == -1)
                    working_ids[i] = context->ScheduleNewResource(config.workingTextures[i], graph_id);
                else
                {
                    working_ids[i] = alias;
                    context->GetScheduledResource(alias).CombineFlags(config.workingTextures[i].flags);
                }
            }

            memcpy(node.parameterIDs, parameter_ids, sizeof(parameter_ids));
            memcpy(node.workingIDs, working_ids, sizeof(working_ids));
            memcpy(node.outputIDs, output_ids, sizeof(output_ids));
            render_flow.push_back(node);
        }

        // Create final graph
        RenderGraph* graph = new RenderGraph();
        graph->m_ID                    = graph_id;
        graph->m_FinalOutputResourceID = m_FinalResourceId;
        graph->m_UnorderedPasses       = used_passes;
        graph->m_RenderFlow            = render_flow;

        // Remove unused passes
        for (auto itr = m_Passes.begin(); itr != m_Passes.end(); ++itr)
        {
            if (used_passes.find((uint32_t)itr->first) == used_passes.end())
            {
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Detected an unused RenderPass in the graph, %d", (uint32_t)itr->first);
                delete itr->second;
            }
        }

        return graph;
    }
}