#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"

#include "d3d12/RenderInterfaceD3D12.h"

namespace RB::Graphics
{
    Shared<GpuGuard> RenderInterface::ExecuteOnGpu()
    {
        m_TotalDraws = 0;
        return ExecuteInternal();
    }

    bool RenderInterface::NeedsIntermediateExecute()
    {
        return m_TotalDraws > INTERMEDIATE_EXECUTE_THRESHOLD;
    }

    void RenderInterface::Draw()
    {
        m_TotalDraws++;
        DrawInternal();

        if (NeedsIntermediateExecute())
        {
            ExecuteOnGpu();
        }
    }

    void RenderInterface::Dispatch(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z)
    {
        m_TotalDraws++;
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
        case RenderAPI::D3D12:
            return new D3D12::RenderInterfaceD3D12(allow_only_copy_operations);
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the render interface for the set graphics API");
            break;
        }

        return nullptr;
    }
}