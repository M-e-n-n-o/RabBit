#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "GraphicsDevice.h"
#include "DeviceQueue.h"
#include "GpuResource.h"
#include "RenderInterfaceVK.h"
#include "RenderResourceVK.h"
#include "ResourceStateManager.h"
#include "UtilsVK.h"

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

        SetNewCommandBuffer();
    }
    
    RenderInterfaceVK::~RenderInterfaceVK()
    {

    }

    Shared<GpuGuard> RenderInterfaceVK::ExecuteInternal()
    {
        FlushAllPending();

        uint64_t submission_value = m_Queue->Submit(m_CommandBuffer);

        SetNewCommandBuffer();

        return CreateShared<GpuGuardVK>(m_Queue, submission_value);
    }

    void RenderInterfaceVK::GpuWaitOn(GpuGuard* guard)
    {
        GpuGuardVK* vk_guard = (GpuGuardVK*)guard;
        m_Queue->GpuWaitForSubmission(vk_guard->m_Queue->GetSemaphore(), vk_guard->m_SubmissionValue);
    }

    void RenderInterfaceVK::TransitionResource(RenderResource* resource, ResourceState state)
    {
        g_ResourceStateManager->TransitionResource(resource, state, m_Queue->GetQueueFamilyIndex());
    }

    void RenderInterfaceVK::FlushResourceBarriers()
    {
        g_ResourceStateManager->FlushPendingTransitions(m_CommandBuffer);
    }

    void RenderInterfaceVK::FlushAllPending()
    {
        FlushResourceBarriers();
    }

    void RenderInterfaceVK::Clear(RenderResource* resource, const Math::Float4& color)
    {
        // TODO: Batch clears together?

        Texture* tex = ((Texture*)resource);

        if (tex->AllowedRenderTarget())
        {
            TransitionResource(resource, ResourceState::COPY_DEST);
            FlushResourceBarriers();

            GpuResource* native_res = (GpuResource*)((VK::Texture2DVK*)resource)->GetNativeResource();

            VkClearColorValue native_color = {};
            native_color.float32[0] = color.r;
            native_color.float32[1] = color.g;
            native_color.float32[2] = color.b;
            native_color.float32[3] = color.a;

            VkImageSubresourceRange color_range = GetImageSubResourceRange(resource);

            vkCmdClearColorImage(m_CommandBuffer, 
                                 native_res->GetNativeImage(),
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 &native_color,
                                 1,
                                 &color_range);
        }
        else if (tex->AllowedDepthStencil())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Depth Stencil clearing not yet implemented");
        }
        else
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not clear RenderResource as its not a rendertarget or a depth stencil");
        }
    }
    
    void RenderInterfaceVK::SetNewCommandBuffer()
    {
        m_CommandBuffer = m_Queue->GetCommandBuffer();
    }
}
#endif