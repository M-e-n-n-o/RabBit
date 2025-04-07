#include "RabBitCommon.h"
#include "RenderGraphContext.h"
#include "RenderResource.h"

namespace RB::Graphics
{
    RenderGraphContext::~RenderGraphContext()
    {
        DeleteGraphResources();
    }

    RenderResource* RenderGraphContext::GetResource(ResourceID id)
    {
        if (id < 0)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderResource from the RenderGraphContext");
            return nullptr;
        }

        return m_Resources[m_ResourcePointers[id]];
    }

    void RenderGraphContext::AddGraphSize(uint32_t graph_id, const RenderGraphSize& size)
    {
        if (graph_id >= m_GraphSizes.size())
        {
            m_GraphSizes.push_back({});
        }

        m_GraphSizes[graph_id].push_back(size);
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

        struct LinkedDesc
        {
            RenderTextureDesc desc;
            uint64_t graphs; // Bitmask for in which graph this desc is used
            List<ResourceID> ids;
        };

        // Will contain all the resources that will be created for this context
        List<LinkedDesc> aliased_descs;

        for (uint32_t current_graph_id = 0; current_graph_id < m_GraphDescriptions.size(); ++current_graph_id)
        {
            RenderGraphSize biggest_size = m_GraphSizes[current_graph_id][0];

            // Create a one fits all sizes
            for (uint32_t i = 1; i < m_GraphSizes[current_graph_id].size(); ++i)
            {
                const RenderGraphSize& current_size = m_GraphSizes[current_graph_id][i];

                biggest_size.size.x = Math::Max(biggest_size.size.x, current_size.size.x);
                biggest_size.size.y = Math::Max(biggest_size.size.y, current_size.size.y);

                biggest_size.uiSize.x = Math::Max(biggest_size.uiSize.x, current_size.uiSize.x);
                biggest_size.uiSize.y = Math::Max(biggest_size.uiSize.y, current_size.uiSize.y);

                biggest_size.upscaledSize.x = Math::Max(biggest_size.upscaledSize.x, current_size.upscaledSize.x);
                biggest_size.upscaledSize.y = Math::Max(biggest_size.upscaledSize.y, current_size.upscaledSize.y);
            }

            for (const ResourceID& current_id : m_GraphDescriptions[current_graph_id])
            {
                RenderTextureDesc current_desc = m_Descriptions[current_id];

                // First make sure to update the size of the description to the actual size
                if (!current_desc.HasFlag(kRTFlag_CustomSized))
                {
                    uint32_t width = (uint32_t)biggest_size.size.x;
                    uint32_t height = (uint32_t)biggest_size.size.y;

                    if (current_desc.HasFlag(kRTFlag_UiSized))
                    {
                        width = (uint32_t)biggest_size.uiSize.x;
                        height = (uint32_t)biggest_size.uiSize.y;
                    }
                    else if (current_desc.HasFlag(kRTFlag_UpscaledSized))
                    {
                        width = (uint32_t)biggest_size.upscaledSize.x;
                        height = (uint32_t)biggest_size.upscaledSize.y;
                    }

                    // Divide the width & height to the desired size
                    current_desc.width = (width >> current_desc.width);
                    current_desc.height = (height >> current_desc.height);
                    current_desc.CombineFlags(kRTFlag_CustomSized);
                }

                RB_ASSERT_FATAL(LOGTAG_GRAPHICS, current_graph_id < 64, "We can not have more than 64 graphs, then the RenderGraph logic will break");

                int32_t aliased_id = -1;

                // Then check if we can reuse a different resource from another graph
                for (uint32_t aliased_idx = 0; aliased_idx < aliased_descs.size(); ++aliased_idx)
                {
                    // Already used by this graph? If so, can not alias
                    if ((aliased_descs[aliased_idx].graphs & (1u << current_graph_id)) != 0)
                        continue;
                    
                    if (aliased_descs[aliased_idx].desc.IsAliasableWith(current_desc))
                    {
                        aliased_id = aliased_idx;
                        break;
                    }

                    // Future improvement would be to alias to a resource with the closest size (only larger or the same ofcourse)
                }

                if (aliased_id != -1)
                {
                    // Found an alias
                    aliased_descs[aliased_id].desc.CombineFlags(current_desc.flags);
                    aliased_descs[aliased_id].graphs |= (1u << current_graph_id);
                    aliased_descs[aliased_id].ids.push_back(current_id);
                    continue;
                }

                // No alias found
                LinkedDesc linked_desc = {};
                linked_desc.desc = current_desc;
                linked_desc.graphs = (1u << current_graph_id);
                linked_desc.ids.push_back(current_id);

                aliased_descs.push_back(linked_desc);
            }
        }

        // Initialize the resource pointers
        size_t size = sizeof(uint32_t) * m_Descriptions.size();
        m_ResourcePointers = (uint32_t*) ALLOC_HEAP(size);
        memset(m_ResourcePointers, 0, size);

        m_Resources.reserve(aliased_descs.size());

        // Actually create the resources
        for (uint32_t i = 0; i < aliased_descs.size(); ++i)
        {
            const LinkedDesc& aliased_desc = aliased_descs[i];

            std::string name = "GraphResouce " + std::to_string(i);

            m_Resources.push_back(Texture2D::Create(name.c_str(),
                                                    aliased_desc.desc.format, 
                                                    aliased_desc.desc.width, 
                                                    aliased_desc.desc.height, 
                                                    aliased_desc.desc.HasFlag(kRTFlag_AllowRenderTarget),
                                                    aliased_desc.desc.HasFlag(kRTFlag_AllowRandomGpuWrites)));

            // Make sure that the ResourceID's point to the correct resource in the m_Resources list
            uint32_t pointer_id = m_Resources.size() - 1;

            for (const ResourceID& current_id : aliased_desc.ids)
            {
                RB_ASSERT_FATAL(LOGTAG_GRAPHICS, m_ResourcePointers[current_id] == 0, "This ResourcePointer is already assigned");
                m_ResourcePointers[current_id] = pointer_id;
            }
        }
    }

    void RenderGraphContext::DeleteGraphResources()
    {
        for (RenderResource* res : m_Resources)
        {
            delete res;
        }

        m_Resources.clear();
        SAFE_FREE(m_ResourcePointers);
    }

    void RenderGraphContext::DeleteGraphResourceDescriptions()
    {
        m_Descriptions.clear();
        m_GraphDescriptions.clear();
    }

    ResourceID RenderGraphContext::ScheduleNewResource(const RenderTextureDesc& desc, uint32_t current_graph_id)
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

    RenderTextureDesc RenderGraphContext::GetScheduledResource(ResourceID id)
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