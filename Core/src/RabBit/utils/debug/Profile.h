#pragma once

#include "Core.h"

#ifdef RB_ENABLE_LOGS
#include <d3d12.h>

#define USE_PIX
#include <pix3.h>
#endif

namespace RB::Utils::Debug
{
#ifdef RB_ENABLE_LOGS

	class RbProfileGpuScoped
	{
	public:
		RbProfileGpuScoped(ID3D12GraphicsCommandList* command_list, uint64_t color, const char* name)
		{
			m_CommandList = command_list;
			PIXBeginEvent(command_list, color, name);
		}

		~RbProfileGpuScoped()
		{
			PIXEndEvent(m_CommandList);
		}

	private:
		ID3D12GraphicsCommandList* m_CommandList;
	};

#define RB_PROFILE_GPU_SCOPED(commandList, name)				RB::Utils::Debug::RbProfileGpuScoped rb_profile_gpu_scoped(commandList, 0, name);
#define RB_PROFILE_GPU_SCOPED_COLOR(commandList, name, color)	RB::Utils::Debug::RbProfileGpuScoped rb_profile_gpu_scoped(commandList, color, name);

#else

#define RB_PROFILE_GPU_SCOPED(commandList, name)		
#define RB_PROFILE_GPU_SCOPED(commandList, name, color) 

#endif
}