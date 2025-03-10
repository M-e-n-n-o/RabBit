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
        if (id < 0 || id >= m_Resources.size())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderResource from the RenderGraphContext");
            return nullptr;
        }

        return m_Resources[id];
    }

    void RenderGraphContext::CreateGraphResources(Math::Float2 size, Math::Float2 upscaled_size, Math::Float2 ui_size)
    {
        // TODO Create the actual resources
    }

    void RenderGraphContext::DeleteGraphResources()
    {
        for (RenderResource* res : m_Resources)
        {
            delete res;
        }

        m_Resources.clear();
    }

    ResourceID RenderGraphContext::ScheduleNewResource(const RenderTextureDesc& desc, uint32_t current_graph_id)
    {
        ResourceID id = -1;

        // Check if we can reuse a different resource from another graph
        for (uint32_t graph_id = 0; graph_id < m_GraphDescriptions.size(); ++graph_id)
        {
            if (graph_id == current_graph_id)
                continue;

            List<ResourceID>& descs = m_GraphDescriptions[graph_id];

            for (uint32_t res_id = 0; res_id < descs.size(); res_id++)
            {
                if (desc.IsAliasableWith(m_Descriptions[descs[res_id]]))
                {
                    id = res_id;
                    break;
                }
            }

            if (id != -1)
                break;
        }

        if (id != -1)
        {
            // Found an alias
            m_GraphDescriptions[current_graph_id].push_back(id);
            CombineScheduledResourceFlags(id, desc.flags);
            return id;
        }

        m_Descriptions.push_back(desc);

        ResourceID new_id = m_Descriptions.size() - 1;

        if (current_graph_id >= m_GraphDescriptions.size())
        {
            m_GraphDescriptions.push_back({});
        }

        m_GraphDescriptions[current_graph_id].push_back(new_id);

        return new_id;
    }

    void RenderGraphContext::CombineScheduledResourceFlags(ResourceID id, uint32_t new_flags)
    {
        if (id < 0 || id >= m_Descriptions.size())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Trying to grab an invalid RenderTextureDesc from the RenderGraphContext");
            return;
        }

        m_Descriptions[id].CombineFlags(new_flags);
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