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
        if (id < 0 || id >= m_ResourcePointers.size())
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

        List<LinkedDesc> aliased_descs;

        
        // First update all the descriptions with the actual sizes of the textures

        // Then alias textures between different graphs

        // Finally actually create all the resources and make sure that the ResourceID's still work


        for (uint32_t current_graph_id = 0; current_graph_id < m_GraphDescriptions.size(); ++current_graph_id)
        {
            for (const RenderGraphSize& current_size : m_GraphSizes[current_graph_id])
            {
                for (const ResourceID& current_id : m_GraphDescriptions[current_graph_id])
                {
                    RenderTextureDesc current_desc = m_Descriptions[current_id];

                    // First make sure to update the size of the description to the actual size
                    if ((current_desc.flags & (uint32_t)RenderTextureFlag::CustomSized) == 0)
                    {
                        uint32_t width = (uint32_t)current_size.size.x;
                        uint32_t height = (uint32_t)current_size.size.y;

                        if ((current_desc.flags & (uint32_t)RenderTextureFlag::UiSized) != 0)
                        {
                            width = (uint32_t)current_size.uiSize.x;
                            height = (uint32_t)current_size.uiSize.y;
                        }
                        else if ((current_desc.flags & (uint32_t)RenderTextureFlag::UpscaledSized) != 0)
                        {
                            width = (uint32_t)current_size.upscaledSize.x;
                            height = (uint32_t)current_size.upscaledSize.y;
                        }

                        current_desc.width = width;
                        current_desc.height = height;
                        current_desc.CombineFlags((uint32_t)RenderTextureFlag::CustomSized);

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
                    }

                    RenderTextureDesc desc = m_Descriptions[current_id];

                    if (aliased_id != -1)
                    {
                        // Found an alias
                        aliased_descs[aliased_id].desc.CombineFlags(desc.flags);
                        aliased_descs[aliased_id].graphs |= (1u << current_graph_id);
                        aliased_descs[aliased_id].ids.push_back(current_id);
                        continue;
                    }

                    // No alias found
                    LinkedDesc linked_desc = {};
                    linked_desc.desc = desc;
                    linked_desc.graphs = (1u << current_graph_id);
                    aliased_descs[aliased_id].ids.push_back(current_id);

                    aliased_descs.push_back(linked_desc);
                }
            }
        }

        // TODO Actually create the resources
        // Make sure that the ResourceID's still work! (using m_ResourcePointers && LinkedDesc::ids)
        // Make sure to fill in the m_ResourcePointers!!!!

        // TODO:
        // - Als een rendergraph meerdere sizes heeft, dan gewoon alleen de grootste sizes maken. Niet zowel de grotere en de kleinere maken! (Misschien dit ook gwn doen voor alle graphs?) (dan kan je zoveel mogelijk textures recyclen)
        // - Zorg dat je een input van een andere ViewContext kan krijgen bij een renderpass
        static_assert(false);
    }

    void RenderGraphContext::DeleteGraphResources()
    {
        for (RenderResource* res : m_Resources)
        {
            delete res;
        }

        m_Resources.clear();
        m_ResourcePointers.clear();
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
        if (graph_id >= m_GraphDescriptions.size())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid list of resource ID's from the RenderGraphContext");
            return {};
        }

        return m_GraphDescriptions[graph_id];
    }
}