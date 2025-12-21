#include "RenderGraphContext.h"
#include "RabBitCommon.h"
#include "RenderGraphContext.h"
#include "RenderResource.h"

namespace RB::Graphics
{
    RenderGraphContext::~RenderGraphContext()
    {
        DeleteGraphResources();
    }

    Shared<RenderResource> RenderGraphContext::GetResource(ResourceID id, uint32_t graph_id, uint16_t size_id)
    {
        if (id < 0)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderResource from the RenderGraphContext");
            return nullptr;
        }

        uint32_t base_idx = m_BasePointers[id];

        uint64_t key = ((uint64_t)base_idx << 32) | ((uint64_t)graph_id << 16) | (uint64_t)size_id;

        auto it = m_AliasLookup.find(key);
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, it != m_AliasLookup.end(), "Alias entry missing for resource:%u graph:%u size:%u", id, graph_id, size_id);

        return m_Resources[it->second];
    }

    Shared<RenderResource> RenderGraphContext::GetResource(ResourceID id)
    {
        if (id < 0)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderResource from the RenderGraphContext");
            return nullptr;
        }

        return m_Resources[m_BasePointers[id]];
    }

    float RenderGraphContext::RequiresClear(ResourceID id)
    {
        if (id < 0)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderResource from the RenderGraphContext");
            return false;
        }

        return m_Clears[m_BasePointers[id]];
    }

    uint16_t RenderGraphContext::AddGraphSize(uint32_t graph_id, const RenderGraphSize& size)
    {
        if (graph_id >= m_GraphSizes.size())
        {
            m_GraphSizes.push_back({});
        }

        m_GraphSizes[graph_id].push_back(size);

        return m_GraphSizes[graph_id].size() - 1;
    }

    void RenderGraphContext::DeleteSizes()
    {
        m_GraphSizes.clear();
    }

    void RenderGraphContext::CreateGraphResources()
    {
        if (m_Resources.size() != 0)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Cannot create graph resources when there are already resources");
            return;
        }

        if (m_GraphSizes.size() == 0)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Cannot create graph resources when there are not sizes registered yet");
            return;
        }

        auto ResolveResourceSize = [](const RenderResourceDesc& desc, const RenderGraphSize& graph_size, uint32_t& out_width, uint32_t& out_height)
        {
            if (desc.HasFlag(kRTFlag_CustomSized))
            {
                // Use absolute dimensions from the desc
                switch (desc.type)
                {
                case RenderResourcePassType::Tex2D:
                    out_width = desc.typeDesc.tex2D.width;
                    out_height = desc.typeDesc.tex2D.height;
                    return;

                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Custom sized resource type not supported");
                    out_width = out_height = 0;
                    return;
                }
            }

            // Pick graph-space size
            uint32_t w = (uint32_t)graph_size.size.x;
            uint32_t h = (uint32_t)graph_size.size.y;

            switch (desc.type)
            {
            case RenderResourcePassType::Tex2D:
                out_width = (w >> desc.typeDesc.tex2D.width);
                out_height = (h >> desc.typeDesc.tex2D.height);
                return;

            default:
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Computed sized resource type not supported");
                out_width = out_height = 0;
                return;
            }
        };

        struct LinkedDesc
        {
            RenderResourceDesc desc;
            uint64_t           graphs; // Bitmask for in which graph this desc is used
            List<ResourceID>   ids;
        };

        // Will contain all the resources that will be created for this context
        List<LinkedDesc> descriptions;

        for (uint32_t current_graph_id = 0; current_graph_id < m_GraphDescriptions.size(); ++current_graph_id)
        {
            RenderGraphSize biggest_size = m_GraphSizes[current_graph_id][0];

            // Create a one fits all sizes
            for (uint32_t i = 1; i < m_GraphSizes[current_graph_id].size(); ++i)
            {
                const RenderGraphSize& current_size = m_GraphSizes[current_graph_id][i];

                biggest_size.size.x = Math::Max(biggest_size.size.x, current_size.size.x);
                biggest_size.size.y = Math::Max(biggest_size.size.y, current_size.size.y);
            }

            for (const ResourceID& current_id : m_GraphDescriptions[current_graph_id])
            {
                RenderResourceDesc current_desc = m_Descriptions[current_id];

                // First make sure to update the size of the description to the actual size
                uint32_t w, h;
                ResolveResourceSize(current_desc, biggest_size, w, h);

                current_desc.typeDesc.tex2D.width = w;
                current_desc.typeDesc.tex2D.height = h;

                RB_ASSERT_FATAL(LOGTAG_GRAPHICS, current_graph_id < 64, "We can not have more than 64 graphs, then the RenderGraph logic will break");

                int32_t aliased_id = -1;

                // Then check if we can reuse a different resource from another graph
                for (uint32_t aliased_idx = 0; aliased_idx < descriptions.size(); ++aliased_idx)
                {
                    // Already used by this graph? If so, can not alias
                    if ((descriptions[aliased_idx].graphs & (1ull << current_graph_id)) != 0)
                        continue;
                    
                    if (descriptions[aliased_idx].desc.IsAliasableWith(current_desc))
                    {
                        aliased_id = aliased_idx;
                        break;
                    }

                    // Future improvement would be to alias to a resource with the closest size (only larger or the same ofcourse)
                }

                if (aliased_id != -1)
                {
                    // Found an alias
                    descriptions[aliased_id].desc.CombineFlags(current_desc.flags);
                    descriptions[aliased_id].graphs |= (1ull << current_graph_id);
                    descriptions[aliased_id].ids.push_back(current_id);
                    continue;
                }

                // No alias found
                LinkedDesc linked_desc = {};
                linked_desc.desc = current_desc;
                linked_desc.graphs = (1ull << current_graph_id);
                linked_desc.ids.push_back(current_id);

                descriptions.push_back(linked_desc);
            }
        }

        // Initialize the resource pointers
        size_t ptr_size = sizeof(uint32_t) * m_Descriptions.size();
        m_BasePointers = (uint32_t*) ALLOC_HEAP(ptr_size);
        memset(m_BasePointers, 0, ptr_size);

        m_Resources.reserve(descriptions.size());
        m_Clears.reserve(descriptions.size());

        // Actually create the resources
        for (uint32_t i = 0; i < descriptions.size(); ++i)
        {
            const LinkedDesc& ld = descriptions[i];

            std::string name = "GraphResouce " + std::to_string(i);

            switch (ld.desc.type)
            {
            case RenderResourcePassType::Tex2D:
            {
                if (ld.desc.typeDesc.tex2D.slices > 1)
                {
                    m_Resources.push_back(Texture2DArray::Create(name.c_str(),
                                                                 ld.desc.format, 
                                                                 ld.desc.typeDesc.tex2D.width, 
                                                                 ld.desc.typeDesc.tex2D.height,
                                                                 ld.desc.typeDesc.tex2D.slices,
                                                                 ld.desc.HasFlag(kRTFlag_AllowRenderTarget),
                                                                 ld.desc.HasFlag(kRTFlag_AllowRandomReadWrites)));
                }
                else
                {
                    m_Resources.push_back(Texture2D::Create(name.c_str(),
                                                            ld.desc.format, 
                                                            ld.desc.typeDesc.tex2D.width,
                                                            ld.desc.typeDesc.tex2D.height,
                                                            ld.desc.HasFlag(kRTFlag_AllowRenderTarget),
                                                            ld.desc.HasFlag(kRTFlag_AllowRandomReadWrites)));
                }
            }
            break;

            default:
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "RenderResourcePassType not yet supported");
                break;
            }

            // Points to the underlying resource (with the biggest size)
            uint32_t base_pointer_id = m_Resources.size() - 1;

            m_Clears.push_back(ld.desc.HasFlag(kRTFlag_ClearBeforeGraph) ? ld.desc.clearValue : -1.0f);

            // Make sure that the ResourceID's point to the correct resource in the m_Resources list
            for (const ResourceID& current_id : ld.ids)
            {
                RB_ASSERT_FATAL(LOGTAG_GRAPHICS, m_BasePointers[current_id] == 0, "This ResourcePointer is already assigned");
                m_BasePointers[current_id] = base_pointer_id;
            }
        }

        m_AliasLookup.clear();
        m_AliasLookup.reserve(m_Descriptions.size() * 2);

        // Create the resolution aliases
        for (uint32_t graph_id = 0; graph_id < m_GraphDescriptions.size(); ++graph_id)
        {
            for (uint32_t size_id = 0; size_id < m_GraphSizes[graph_id].size(); ++size_id)
            {
                const RenderGraphSize& sz = m_GraphSizes[graph_id][size_id];

                // For each resource used by this graph
                for (ResourceID rid : m_GraphDescriptions[graph_id])
                {
                    uint32_t base_idx = m_BasePointers[rid];
                    auto base = m_Resources[base_idx];

                    const RenderResourceDesc& scheduled_desc = m_Descriptions[rid];

                    uint32_t resolved_width, resolved_height;
                    ResolveResourceSize(scheduled_desc, sz, resolved_width, resolved_height);

                    // Create alias from the base resource and set viewport sizes
                    Shared<RenderResource> alias = nullptr;

                    switch (base->GetType())
                    {
                    case RenderResourceType::Texture2D:
                    {
                        auto a = Texture2D::Alias(std::static_pointer_cast<Texture2D>(base));
                        a->SetViewportWidth(resolved_width);
                        a->SetViewportHeight(resolved_height);
                        alias = a;
                    }
                    break;

                    case RenderResourceType::Texture2DArray:
                    {
                        auto a = Texture2DArray::Alias(std::static_pointer_cast<Texture2DArray>(base));
                        a->SetViewportWidth(resolved_width);
                        a->SetViewportHeight(resolved_height);
                        alias = a;
                    }
                    break;

                    default:
                        RB_LOG_ERROR(LOGTAG_GRAPHICS, "Alias creation unsupported type");
                        continue;
                    }

                    uint32_t alias_idx = m_Resources.size();
                    m_Resources.push_back(alias);

                    uint64_t key = ((uint64_t)base_idx << 32) | ((uint64_t)graph_id << 16) | (uint64_t)size_id;
                    m_AliasLookup[key] = alias_idx;
                }
            }
        }
    }

    void RenderGraphContext::DeleteGraphResources()
    {
        m_AliasLookup.clear();
        m_Resources.clear();
        m_Clears.clear();
        SAFE_FREE(m_BasePointers);
    }

    void RenderGraphContext::DeleteGraphResourceDescriptions()
    {
        m_Descriptions.clear();
        m_GraphDescriptions.clear();
    }

    ResourceID RenderGraphContext::ScheduleNewResource(const RenderResourceDesc& desc, uint32_t current_graph_id)
    {
        if (current_graph_id >= m_GraphDescriptions.size())
        {
            m_GraphDescriptions.push_back({});
        }

        m_Descriptions.push_back(desc);

        ResourceID new_id = m_Descriptions.size() - 1;

        m_GraphDescriptions[current_graph_id].push_back(new_id);

        return new_id;
    }

    RenderResourceDesc RenderGraphContext::GetScheduledResource(ResourceID id)
    {
        if (id < 0 || id >= m_Descriptions.size())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderTextureDesc from the RenderGraphContext");
            return {};
        }

        return m_Descriptions[id];
    }

    List<ResourceID> RenderGraphContext::GetScheduledGraphResources(uint32_t graph_id)
    {
        if (m_GraphDescriptions.empty())
        {
            return {};
        }

        if (graph_id >= m_GraphDescriptions.size())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid list of resource ID's from the RenderGraphContext");
            return {};
        }

        return m_GraphDescriptions[graph_id];
    }
}