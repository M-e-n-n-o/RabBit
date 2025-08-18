#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "RabBitCommon.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class DeviceQueue
    {
    public:
        DeviceQueue(VkDevice device, VkQueueFlags type, uint32_t queue_family_index, uint32_t queue_index);
        ~DeviceQueue();

        VkCommandBuffer GetCommandBuffer();

        uint64_t Submit(VkCommandBuffer command_buffer);

        void CpuWaitUntilIdle(uint64_t max_duration_ms = UINT64_MAX);
        void CpuWaitForSubmission(uint64_t submission_value, uint64_t max_duration_ms = UINT64_MAX);
        void GpuWaitForSubmission(VkSemaphore other_semaphore, uint64_t submission_value);

        bool IsSubmissionComplete(uint64_t submission_value);

        VkQueue GetQueue() const { return m_Queue; }
        uint32_t GetQueueFamilyIndex() const { return m_QueueFamilyIndex; }

        VkSemaphore GetSemaphore() const { return m_TimelineSemaphore; }

    private:
        void UpdateRunningSubmissions();
        void CreateFence();
        void CreateTimelineSemaphore(VkDevice device);

        struct CommandSubmission
        {
            VkCommandBuffer commandBuffer;
            uint64_t submissionValue;
        };

        VkQueueFlags            m_Type;
        uint32_t                m_QueueFamilyIndex;
        VkQueue                 m_Queue;

        // TODO: If we ever want to add multithreaded command recording,
        // then we should have 1 commandPool for every thread!
        VkCommandPool           m_CommandPool;
        Queue<VkCommandBuffer>  m_AvailableCommandBuffers;
        List<CommandSubmission> m_RunningSubmissions;

        uint64_t                m_LastSignaledValue;
        VkSemaphore             m_TimelineSemaphore;
    };
}
#endif