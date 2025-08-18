#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "DeviceQueue.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    DeviceQueue::DeviceQueue(VkDevice device, VkQueueFlags type, uint32_t queue_family_index, uint32_t queue_index)
        : m_Type(type)
        , m_QueueFamilyIndex(queue_family_index)
    {
        vkGetDeviceQueue(device, queue_family_index, queue_index, &m_Queue);

        // Create a single command pool
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType             = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex  = queue_family_index;
        pool_info.flags             = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateCommandPool(device, &pool_info, nullptr, &m_CommandPool),
            "Failed to create command pool");

        CreateTimelineSemaphore(device);
    }

    DeviceQueue::~DeviceQueue()
    {
        CpuWaitUntilIdle();

        if (m_CommandPool)
            vkDestroyCommandPool(g_GraphicsDevice->Get(), m_CommandPool, nullptr);

        if (m_TimelineSemaphore)
            vkDestroySemaphore(g_GraphicsDevice->Get(), m_TimelineSemaphore, nullptr);
    }

    VkCommandBuffer DeviceQueue::GetCommandBuffer()
    {
        VkCommandBuffer buffer;

        UpdateRunningSubmissions();

        if (m_AvailableCommandBuffers.empty())
        {
            VkCommandBufferAllocateInfo alloc_info = {};
            alloc_info.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool          = m_CommandPool;
            alloc_info.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount   = 1;

            RB_ASSERT_FATAL_RELEASE_VK(vkAllocateCommandBuffers(g_GraphicsDevice->Get(), &alloc_info, &buffer),
                "Failed to allocate command buffer");
        }
        else
        {
            buffer = m_AvailableCommandBuffers.front();
            m_AvailableCommandBuffers.pop();

            RB_ASSERT_FATAL_RELEASE_VK(vkResetCommandBuffer(buffer, 0), 
                "Failed to reset command buffer");
        }

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        RB_ASSERT_FATAL_RELEASE_VK(vkBeginCommandBuffer(buffer, &begin_info),
            "Failed to begin command buffer");

        return buffer;
    }

    uint64_t DeviceQueue::Submit(VkCommandBuffer command_buffer)
    {
        RB_ASSERT_FATAL_RELEASE_VK(vkEndCommandBuffer(command_buffer), "Failed to end command buffer");

        uint64_t signal_value = ++m_LastSignaledValue;

        VkTimelineSemaphoreSubmitInfo timeline_info = {};
        timeline_info.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timeline_info.signalSemaphoreValueCount = 1;
        timeline_info.pSignalSemaphoreValues    = &signal_value;

        VkSubmitInfo submit_info = {};
        submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext                   = &timeline_info;
        submit_info.commandBufferCount      = 1;
        submit_info.pCommandBuffers         = &command_buffer;
        submit_info.signalSemaphoreCount    = 1;
        submit_info.pSignalSemaphores       = &m_TimelineSemaphore;

        RB_ASSERT_FATAL_RELEASE_VK(vkQueueSubmit(m_Queue, 1, &submit_info, VK_NULL_HANDLE),
            "Failed to submit command buffer");

        m_RunningSubmissions.push_back({ command_buffer, signal_value });

        return signal_value;
    }

    void DeviceQueue::UpdateRunningSubmissions()
    {
        auto itr = m_RunningSubmissions.begin();
        while (itr != m_RunningSubmissions.end())
        {
            if (IsSubmissionComplete(itr->submissionValue))
            {
                m_AvailableCommandBuffers.push(itr->commandBuffer);
                itr = m_RunningSubmissions.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }

    void DeviceQueue::CpuWaitUntilIdle(uint64_t max_duration_ms)
    {
        if (m_LastSignaledValue == 0)
        {
            vkQueueWaitIdle(m_Queue);
            return;
        }

        CpuWaitForSubmission(m_LastSignaledValue, max_duration_ms);
    }

    void DeviceQueue::CpuWaitForSubmission(uint64_t submission_value, uint64_t max_duration_ms)
    {
        if (IsSubmissionComplete(submission_value))
            return;

        VkSemaphoreWaitInfo wait_info = {};
        wait_info.sType             = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wait_info.semaphoreCount    = 1;
        wait_info.pSemaphores       = &m_TimelineSemaphore;
        wait_info.pValues           = &submission_value;

        uint64_t duration_ns;
        if (max_duration_ms == UINT64_MAX)
            duration_ns = UINT64_MAX;
        else
            duration_ns = max_duration_ms * 1000000; // This might overflow

        RB_ASSERT_FATAL_RELEASE_VK(vkWaitSemaphores(g_GraphicsDevice->Get(), &wait_info, duration_ns), "Failed to wait on semaphore");
    }

    void DeviceQueue::GpuWaitForSubmission(VkSemaphore other_semaphore, uint64_t submission_value)
    {
        VkTimelineSemaphoreSubmitInfo timeline_info = {};
        timeline_info.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timeline_info.waitSemaphoreValueCount   = 1;
        timeline_info.pWaitSemaphoreValues      = &submission_value;

        VkPipelineStageFlags wait_stage;
        if (m_Type & VK_QUEUE_GRAPHICS_BIT)
            wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        else if (m_Type & VK_QUEUE_COMPUTE_BIT)
            wait_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
        else if (m_Type & VK_QUEUE_TRANSFER_BIT)
            wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        else
            wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Do an empty submit
        VkSubmitInfo submit_info = {};
        submit_info.sType               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext               = &timeline_info;
        submit_info.waitSemaphoreCount  = 1;
        submit_info.pWaitSemaphores     = &other_semaphore;
        submit_info.pWaitDstStageMask   = &wait_stage;
        submit_info.commandBufferCount  = 0;
        submit_info.pCommandBuffers     = nullptr;

        RB_ASSERT_FATAL_RELEASE_VK(vkQueueSubmit(m_Queue, 1, &submit_info, VK_NULL_HANDLE), "GPU wait failed");
    }

    bool DeviceQueue::IsSubmissionComplete(uint64_t submission_value)
    {
        uint64_t completed_value;
        RB_ASSERT_FATAL_RELEASE_VK(vkGetSemaphoreCounterValue(g_GraphicsDevice->Get(), m_TimelineSemaphore, &completed_value),
            "Failed to get semaphore value");

        return completed_value >= submission_value;
    }

    void DeviceQueue::CreateTimelineSemaphore(VkDevice device)
    {
        m_LastSignaledValue = 0;

        VkSemaphoreTypeCreateInfo timeline_info = {};
        timeline_info.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timeline_info.initialValue  = m_LastSignaledValue;

        VkSemaphoreCreateInfo sem_info = {};
        sem_info.sType              = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem_info.pNext              = &timeline_info;

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateSemaphore(device, &sem_info, nullptr, &m_TimelineSemaphore),
            "Failed to create timeline semaphore");
    }
}
#endif