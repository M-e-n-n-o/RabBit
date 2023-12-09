#include "RabBitCommon.h"
#include "Pipeline.h"
#include "graphics/native/GraphicsDevice.h"

namespace RB::Graphics::Native
{
	template <class T>
	inline void HashCombine(uint64_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	PipelineManager* g_PipelineManager = nullptr;

	PipelineManager::PipelineManager()
	{

	}

	GPtr<ID3D12PipelineState> PipelineManager::GetComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
	{
		uint64_t hash = GetPipelineHash(desc);

		auto found = m_ComputePipelines.find(hash);
		if (found != m_ComputePipelines.end())
		{
			return found->second;
		}

		GPtr<ID3D12PipelineState> pso;
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)), "Could not create compute pso");
		
		m_ComputePipelines.insert({ hash, pso });

		return pso;
	}

	GPtr<ID3D12PipelineState> PipelineManager::GetGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		uint64_t hash = GetPipelineHash(desc);

		auto found = m_GraphicsPipelines.find(hash);
		if (found != m_GraphicsPipelines.end())
		{
			return found->second;
		}

		GPtr<ID3D12PipelineState> pso;
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)), "Could not create graphics pso");

		m_GraphicsPipelines.insert({ hash, pso });

		return pso;
	}

	uint64_t PipelineManager::GetPipelineHash(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
	{
		uint64_t seed = 0;

		//HashCombine(seed, (char*) desc.CS.pShaderBytecode);
		HashCombine(seed, desc.NodeMask);
		HashCombine(seed, (char*) desc.CachedPSO.pCachedBlob);
		HashCombine(seed, (UINT) desc.Flags);
		
		//TODO desc.pRootSignature

		return seed;
	}

	uint64_t PipelineManager::GetPipelineHash(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		// TODO, finish this!

		//uint64_t seed = 0;

		////HashCombine(seed, (char*) desc.VS.pShaderBytecode);
		////HashCombine(seed, (char*) desc.PS.pShaderBytecode);
		////HashCombine(seed, (char*) desc.DS.pShaderBytecode);
		////HashCombine(seed, (char*) desc.HS.pShaderBytecode);
		////HashCombine(seed, (char*) desc.GS.pShaderBytecode);
		////HashCombine(seed, desc.StreamOutput);
		//HashCombine(seed, desc.BlendState.AlphaToCoverageEnable);
		//HashCombine(seed, desc.BlendState.IndependentBlendEnable);
		////HashCombine(seed, desc.BlendState.RenderTarget);
		//HashCombine(seed, desc.SampleMask);
		//HashCombine(seed, desc.RasterizerState);

		//return seed;

		return m_GraphicsPipelines.size();
	}
}