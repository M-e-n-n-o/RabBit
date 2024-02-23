#pragma once

#include "RabBitCommon.h"
#include "graphics/d3d12/shaders/ShaderSystem.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class PipelineManager
	{
	public:
		PipelineManager();

		GPtr<ID3D12PipelineState> GetComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);
		GPtr<ID3D12PipelineState> GetGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);

	private:
		uint64_t GetPipelineHash(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);
		uint64_t GetPipelineHash(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);

		UnorderedMap<uint64_t, GPtr<ID3D12PipelineState>> m_ComputePipelines;
		UnorderedMap<uint64_t, GPtr<ID3D12PipelineState>> m_GraphicsPipelines;
	};

	extern PipelineManager* g_PipelineManager;
}