#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "ResourceStateManager.h"
#include "GpuResource.h"
#include "UtilsVK.h"

namespace RB::Graphics::VK
{
    ResourceStateManager* g_ResourceStateManager = nullptr;

    ResourceStateManager::ResourceStateManager()
    {
    }

    void ResourceStateManager::TransitionResource(RenderResource* resource, ResourceState new_state, uint32_t queue_family)
    {
        GpuResource* native_res = (GpuResource*)resource->GetNativeResource();

        bool needs_state_change = native_res->GetState() != new_state;

        uint32_t current_family = native_res->GetCurrentQueueOwnership();
        bool need_transfer = (queue_family != current_family && native_res->IsOwnedByQueue());
        native_res->SetQueueOwnership(queue_family);

        if (!needs_state_change && !need_transfer)
        {
            return;
        }

        VkAccessFlags src_access, dst_access;
        VkPipelineStageFlags src_stage, dst_stage;
        VkImageLayout old_layout, new_layout;

        GetAccessMasksForState(native_res->GetState(), src_access, src_stage, old_layout);
        GetAccessMasksForState(new_state, dst_access, dst_stage, new_layout);

        native_res->UpdateState(new_state);

        if (native_res->GetType() == GpuResourceType::Image)
        {
            VkImageMemoryBarrier barrier = {};
            barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask       = needs_state_change ? src_access : (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
            barrier.dstAccessMask       = needs_state_change ? dst_access : (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
            barrier.oldLayout           = old_layout;
            barrier.newLayout           = new_layout;
            barrier.srcQueueFamilyIndex = need_transfer ? current_family : VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = need_transfer ? queue_family   : VK_QUEUE_FAMILY_IGNORED;
            barrier.image               = native_res->GetNativeImage();
            barrier.subresourceRange    = GetImageSubResourceRange(resource);

            m_PendingImageBarriers.push_back({ src_stage, dst_stage, barrier });
        }
        else
        {
            VkBufferMemoryBarrier barrier = {};
            barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask       = needs_state_change ? src_access : (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
            barrier.dstAccessMask       = needs_state_change ? dst_access : (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
            barrier.srcQueueFamilyIndex = need_transfer ? current_family : VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = need_transfer ? queue_family   : VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer              = native_res->GetNativeBuffer();
            barrier.offset              = 0;
            barrier.size                = VK_WHOLE_SIZE;

            m_PendingBufferBarriers.push_back({ src_stage, dst_stage, barrier });
        }
    }

    void ResourceStateManager::FlushPendingTransitions(VkCommandBuffer cmd)
    {
        if (m_PendingImageBarriers.empty() && m_PendingBufferBarriers.empty())
        {
            return;
        }

        std::vector<VkImageMemoryBarrier> image_barriers;
        std::vector<VkBufferMemoryBarrier> buffer_barriers;

        VkPipelineStageFlags src_stage_mask = 0;
        VkPipelineStageFlags dst_stage_mask = 0;

        for (auto& b : m_PendingImageBarriers)
        {
            src_stage_mask |= b.srcStageMask;
            dst_stage_mask |= b.dstStageMask;
            image_barriers.push_back(b.barrier);
        }
        for (auto& b : m_PendingBufferBarriers)
        {
            src_stage_mask |= b.srcStageMask;
            dst_stage_mask |= b.dstStageMask;
            buffer_barriers.push_back(b.barrier);
        }

        vkCmdPipelineBarrier(
            cmd,
            src_stage_mask ? src_stage_mask : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            dst_stage_mask ? dst_stage_mask : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            static_cast<uint32_t>(buffer_barriers.size()), buffer_barriers.data(),
            static_cast<uint32_t>(image_barriers.size()), image_barriers.data()
        );

        m_PendingImageBarriers.clear();
        m_PendingBufferBarriers.clear();
    }
}
#endif