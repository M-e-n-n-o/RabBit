#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "RenderInterfaceVK.h"
#include "GraphicsDevice.h"
#include "DeviceQueue.h"

namespace RB::Graphics::VK
{
    // ---------------------------------------------------------------------------
    //                                GpuGuard
    // ---------------------------------------------------------------------------

    GpuGuardVK::GpuGuardVK(DeviceQueue* queue, uint64_t submission_value)
        : m_Queue(queue)
        , m_SubmissionValue(submission_value)
    {
    }

    bool GpuGuardVK::IsFinishedRendering()
    {
        return m_Queue->IsSubmissionComplete(m_SubmissionValue);
    }

    void GpuGuardVK::WaitUntilFinishedRendering()
    {
        m_Queue->CpuWaitForSubmission(m_SubmissionValue);
    }

    // ---------------------------------------------------------------------------
    //                             RenderInterface
    // ---------------------------------------------------------------------------

    RenderInterfaceVK::RenderInterfaceVK(bool allow_only_copy_operations)
        : m_CopyOperationsOnly(allow_only_copy_operations)
    {
        if (allow_only_copy_operations)
            m_Queue = g_GraphicsDevice->GetTransferQueue();
        else
            m_Queue = g_GraphicsDevice->GetGraphicsQueue();

        // TODO set actual command buffer
        static_assert(false);
    }
    
    RenderInterfaceVK::~RenderInterfaceVK()
    {
        // TODO
    }

    Shared<GpuGuard> RenderInterfaceVK::ExecuteInternal()
    {
        uint64_t submission_value = m_Queue->Submit(m_CommandBuffer);

        // TODO Refresh the command buffer
        static_assert(false);

        return CreateShared<GpuGuardVK>(m_Queue, submission_value);
    }

    void RenderInterfaceVK::GpuWaitOn(GpuGuard* guard)
    {
        GpuGuardVK* vk_guard = (GpuGuardVK*)guard;
        m_Queue->GpuWaitForSubmission(vk_guard->m_Queue->GetSemaphore(), vk_guard->m_SubmissionValue);
    }
}
#endif