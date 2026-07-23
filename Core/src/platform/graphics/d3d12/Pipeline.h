#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"

#include "graphics/ShaderSystem.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class PipelineManager
    {
    public:
        PipelineManager();

        GPtr<ID3D12PipelineState> GetComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, uint32_t cs_identifier);
        GPtr<ID3D12PipelineState> GetGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint32_t vs_identifier, int32_t ps_identifier);

        GPtr<ID3D12RootSignature> GetRootSignature(const ShaderSystem* ss, uint32_t vs_identifier, int32_t ps_identifier);
        GPtr<ID3D12RootSignature> GetRootSignature(const ShaderSystem* ss, uint32_t cs_identifier);

        List<D3D12_INPUT_ELEMENT_DESC> GetInputElementDesc(const ShaderSystem* ss, uint32_t vs_identifier, uint32_t vertex_buffers_count);

        uint32_t GetRootSignatureCbvBindingOffset(uint32_t vs_identifier, int32_t ps_identifier) const;
        uint32_t GetRootSignatureCbvBindingOffset(uint32_t cs_identifier) const;

    private:        
        uint64_t GetPipelineHash(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, uint64_t root_signature_hash);
        uint64_t GetPipelineHash(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint64_t root_signature_hash);

        List<D3D12_STATIC_SAMPLER_DESC> GetSamplerDescriptions(const ShaderSystem* ss, uint32_t shader_identifier);

        UnorderedMap<uint64_t, GPtr<ID3D12PipelineState>>       m_ComputePipelines;
        UnorderedMap<uint64_t, GPtr<ID3D12PipelineState>>       m_GraphicsPipelines;

        UnorderedMap<uint64_t, GPtr<ID3D12RootSignature>>       m_RootSignatures;
        UnorderedMap<uint64_t, uint32_t>                        m_CbvOffsets;
        UnorderedMap<uint64_t, List<D3D12_INPUT_ELEMENT_DESC>>  m_InputElementDescriptions;
    };

    extern PipelineManager* g_PipelineManager;
}
#endif