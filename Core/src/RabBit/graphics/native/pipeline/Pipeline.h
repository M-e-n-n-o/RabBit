#pragma once

#include "RabBitCommon.h"

#include <d3d12.h>

namespace RB::Graphics::Native
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

	// ------------------------------------------------------------------
	//							Pipeline Helpers
	// ------------------------------------------------------------------

	//class GraphicsPipelineBuilder
	//{
	//	GraphicsPipelineBuilder();

	//	void SetVertexShader();
	//	void SetPixelShader();

	//	void SetInputLayout();
	//	void SetInputLayout(/* vao */);

	//	// Check if the minimum parameters are set!
	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC Build();

	//private:
	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_Desc;
	//};

	//class ComputePipelineBuilder
	//{
	//	ComputePipelineBuilder();

	//	// Check if the minimum parameters are set!
	//	D3D12_COMPUTE_PIPELINE_STATE_DESC Build();

	//private:
	//	D3D12_COMPUTE_PIPELINE_STATE_DESC m_Desc;
	//};
}