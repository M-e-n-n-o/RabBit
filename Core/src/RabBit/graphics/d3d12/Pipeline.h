#pragma once

#include "RabBitCommon.h"

#include <d3d12.h>

namespace RB::Graphics
{
	class ShaderSystem;
}

namespace RB::Graphics::D3D12
{

	// CBV's are the first parameters in the root signature
	#define CBV_ROOT_PARAMETER_INDEX_OFFSET 0

	struct RootSignatureParameterDescriptorDesc
	{
		// Max value of 32!
		uint32_t rootIndexSlotsUsed;

		// Bit masks indicating on which root index which type of descriptor is bound
		uint32_t descriptorTableBitMask;
		uint32_t samplerTableBitMask;
		uint32_t cbvBitMask;

		// Array sorted by root index, storing the amount of descriptors per table, 
		// if there is a desctiptor table at that index.
		uint32_t numDescriptorsPerTable[32 /* (sizeof(uint32_t) * 8) */ ];
	};

	class PipelineManager
	{
	public:
		PipelineManager(ShaderSystem* shader_system);

		GPtr<ID3D12PipelineState> GetComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, uint32_t cs_identifier);
		GPtr<ID3D12PipelineState> GetGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint32_t vs_identifier, uint32_t ps_identifier);

		GPtr<ID3D12RootSignature> GetRootSignature(uint32_t vs_identifier, uint32_t ps_identifier);

		List<D3D12_INPUT_ELEMENT_DESC> GetInputElementDesc(uint32_t vs_identifier);

	private:
		uint64_t GetPipelineHash(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, uint64_t root_signature_hash);
		uint64_t GetPipelineHash(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint64_t root_signature_hash);

		UnorderedMap<uint64_t, GPtr<ID3D12PipelineState>>		m_ComputePipelines;
		UnorderedMap<uint64_t, GPtr<ID3D12PipelineState>>		m_GraphicsPipelines;

		UnorderedMap<uint64_t, GPtr<ID3D12RootSignature>>		m_RootSignatures;
		UnorderedMap<uint32_t, List<D3D12_INPUT_ELEMENT_DESC>>	m_InputElementDescriptions;

		ShaderSystem*											m_ShaderSystem;
	};

	extern PipelineManager* g_PipelineManager;
}