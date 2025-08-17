#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class GpuResource; // Forward declaration

    class ResourceStateManager
    {
    public:
        ResourceStateManager();

        void TransitionResource(GpuResource* resource, ResourceState new_state);

        void FlushPendingTransitions(VkCommandBuffer cmd);

    private:
        static void GetAccessMasksForState(ResourceState state,
                                           VkAccessFlags& access_mask,
                                           VkPipelineStageFlags& stage_mask,
                                           VkImageLayout& layout);

        struct PendingImageBarrier
        {
            VkPipelineStageFlags srcStageMask;
            VkPipelineStageFlags dstStageMask;
            VkImageMemoryBarrier barrier;
        };

        struct PendingBufferBarrier
        {
            VkPipelineStageFlags  srcStageMask;
            VkPipelineStageFlags  dstStageMask;
            VkBufferMemoryBarrier barrier;
        };

        List<PendingImageBarrier>  m_PendingImageBarriers;
        List<PendingBufferBarrier> m_PendingBufferBarriers;
    };

    extern ResourceStateManager* g_ResourceStateManager;
}
#endif