#include "RabBitCommon.h"
#include "Pipeline.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
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

	// TODO Put this in a render interface, as we can not create an entire pipeline from just the shader blobs
	//void PipelineManager::CreateGraphicsPipelineFromShaderBlobs(ShaderBlob* vertex_blob, ShaderBlob* pixel_blob)
	//{
	//	ID3D12ShaderReflection* vref = vertex_blob->m_ReflectionData.Get();
	//	ID3D12ShaderReflection* pref = pixel_blob->m_ReflectionData.Get();

	//	D3D12_SHADER_DESC vdesc;
	//	vref->GetDesc(&vdesc);

	//	D3D12_SHADER_DESC pdesc;
	//	pref->GetDesc(&pdesc);

	//	RB_ASSERT_FATAL(LOGTAG_GRAPHICS, (D3D12_SHADER_VERSION_TYPE)((vdesc.Version & 0xFFFF0000) >> 16) == D3D12_SHADER_VERSION_TYPE::D3D12_SHVER_VERTEX_SHADER,
	//		"Can not create pipeline from shader blob as the inputted vertex shader is not a vertex shader");
	//	RB_ASSERT_FATAL(LOGTAG_GRAPHICS, (D3D12_SHADER_VERSION_TYPE)((pdesc.Version & 0xFFFF0000) >> 16) == D3D12_SHADER_VERSION_TYPE::D3D12_SHVER_PIXEL_SHADER,
	//		"Can not create pipeline from shader blob as the inputted pixel shader is not a pixel shader");

	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
	//	pso.NodeMask			= 0;
	//	pso.VS					= { vertex_blob->m_ShaderBlob, vertex_blob->m_ShaderBlobSize };
	//	pso.PS					= { pixel_blob->m_ShaderBlob, pixel_blob->m_ShaderBlobSize };
	//	pso.BlendState			= 
	//}

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

		// Do not hash the shader blob, but just simply use the shader ID as that is unique to each shader!!!
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