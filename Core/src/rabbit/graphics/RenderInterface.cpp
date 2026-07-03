#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"

#if RB_GRAPHICS_API_D3D12
#include "platform/graphics/d3d12/RenderInterfaceD3D12.h"
#endif

#if RB_GRAPHICS_API_VULKAN
#include "platform/graphics/vulkan/RenderInterfaceVK.h"
#endif

namespace RB::Graphics
{
    Shared<GpuGuard> RenderInterface::ExecuteOnGpu()
    {
        return ExecuteInternal();
    }

    bool RenderInterface::NeedsIntermediateExecute()
    {
        return false; //m_TotalDraws > INTERMEDIATE_EXECUTE_THRESHOLD;
    }

    void RenderInterface::Draw()
    {
        DrawInternal();

        if (NeedsIntermediateExecute())
        {
            ExecuteOnGpu();
        }
    }

    void RenderInterface::DrawInstanced(uint32_t instances)
    {
        DrawInstancedInternal(instances);

        if (NeedsIntermediateExecute())
        {
            ExecuteOnGpu();
        }
    }

    void RenderInterface::Dispatch(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z)
    {
        DispatchInternal(thread_groups_x, thread_groups_y, thread_groups_z);

        if (NeedsIntermediateExecute())
        {
            ExecuteOnGpu();
        }
    }

    void RenderInterface::Clear(RenderResource* resource)
    {
        Clear(resource, Math::Float4(0, 0, 0, 0));
    }

    void RenderInterface::ClearDepth(RenderResource* resource, bool reversed_depth)
    {
        Clear(resource, Math::Float4(reversed_depth ? 0 : 1, 0, 0, 0));
    }

    RenderInterface* RenderInterface::Create(bool allow_only_copy_operations)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return new D3D12::RenderInterfaceD3D12(allow_only_copy_operations);
#endif

#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
            return new VK::RenderInterfaceVK(allow_only_copy_operations);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the render interface for the set graphics API");
            break;
        }

        return nullptr;
    }
}