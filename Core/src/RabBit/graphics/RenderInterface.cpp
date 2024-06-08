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

	void RenderInterface::Dispatch()
	{
		m_TotalDraws++;
		DispatchInternal();

		if (NeedsIntermediateExecute())
		{
			ExecuteOnGpu();
		}
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