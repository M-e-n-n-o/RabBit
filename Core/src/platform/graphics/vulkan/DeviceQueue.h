#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "RabBitCommon.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class DeviceQueue
    {
    public:
        DeviceQueueVK(VkQueueFlags type, uint32_t queue_family_index, uint32_t queue_index);
        ~DeviceQueueVK();

        VkQueueFlags GetType() const { return m_Type; }
        uint32_t GetQueueFamilyIndex() const { return m_QueueFamilyIndex; }

        uint64_t SignalFence();
        bool IsFenceCompleted(uint64_t fence_value);
        void CpuWaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms = std::numeric_limits<uint64_t>::max());
        void GpuWaitForFenceValue(uint64_t fence_value);
        void CpuWaitUntilIdle();

        VkCommandBuffer GetCommandBuffer();
        uint64_t ExecuteCommandBuffer(VkCommandBuffer cmd_buffer, bool wait_for_completion = false);

        VkQueue GetQueue() const { return m_Queue; }

    private:
        struct CommandPoolEntry
        {
            uint64_t fence_value;
            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
        };

        void CreateFence();
        void UpdateCompletedCommandPools();

        VkQueueFlags                        m_Type;
        uint32_t                            m_QueueFamilyIndex;
        VkQueue                             m_Queue;
        VkFence                             m_Fence;
        uint64_t                            m_FenceValue;

        List<VkCommandBuffer>               m_AvailableCommandBuffers;
        List<CommandPoolEntry>              m_ActiveCommandPools;
        Map<VkCommandBuffer, VkCommandPool> m_CommandBufferToPool;
    };
}
#endif