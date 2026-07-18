#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "Pipeline.h"
#include "GraphicsDevice.h"
#include "resource/Descriptor.h"
#include "graphics/ShaderSystem.h"

#include <d3dx12/d3dx12.h>

using namespace RB::ShaderCompiler;

namespace RB::Graphics::D3D12
{
    PipelineManager* g_PipelineManager = nullptr;

    PipelineManager::PipelineManager()
    {
        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, g_GraphicsDevice->IsFeatureSupported(D3D12_RESOURCE_BINDING_TIER_3) && g_GraphicsDevice->IsFeatureSupported(D3D_SHADER_MODEL_6_6),
            "Resource binding tier 3 or shader model 6.6 is not supported on the device");
    }

    GPtr<ID3D12PipelineState> PipelineManager::GetComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, uint32_t cs_identifier)
    {
        uint64_t rs_hash = 0;
        HashCombine(rs_hash, cs_identifier);

        uint64_t hash = GetPipelineHash(desc, rs_hash);

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

    GPtr<ID3D12PipelineState> PipelineManager::GetGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint32_t vs_identifier, int32_t ps_identifier)
    {
        //	RB_ASSERT_FATAL(LOGTAG_GRAPHICS, (D3D12_SHADER_VERSION_TYPE)((vdesc.Version & 0xFFFF0000) >> 16) == D3D12_SHADER_VERSION_TYPE::D3D12_SHVER_VERTEX_SHADER,
        //		"Can not create pipeline from shader blob as the inputted vertex shader is not a vertex shader");
        //	RB_ASSERT_FATAL(LOGTAG_GRAPHICS, (D3D12_SHADER_VERSION_TYPE)((pdesc.Version & 0xFFFF0000) >> 16) == D3D12_SHADER_VERSION_TYPE::D3D12_SHVER_PIXEL_SHADER,
        //		"Can not create pipeline from shader blob as the inputted pixel shader is not a pixel shader");

        uint64_t rs_hash = 0;
        HashCombine(rs_hash, vs_identifier);
        HashCombine(rs_hash, ps_identifier);

        uint64_t hash = GetPipelineHash(desc, rs_hash);

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

    GPtr<ID3D12RootSignature> PipelineManager::GetRootSignature(const ShaderSystem* ss, uint32_t vs_identifier, int32_t ps_identifier)
    {
        uint64_t hash = 0;
        HashCombine(hash, vs_identifier);
        HashCombine(hash, ps_identifier);

        auto found = m_RootSignatures.find(hash);

        if (found != m_RootSignatures.end())
        {
            return found->second;
        }

        const auto* vs_reflection = ss->GetReflection(vs_identifier);
        const auto* ps_reflection = ss->GetReflection(ps_identifier);

        List<CD3DX12_ROOT_PARAMETER1> parameters;

        // Set entry point parameters as root constants
        {
            uint32_t vs_size = 0;
            for (int i = 0; i < vs_reflection->entryPointParameters.size(); i++)
            {
                vs_size += vs_reflection->entryPointParameters[i].size;
            }
            if (vs_size > 0)
            {
                CD3DX12_ROOT_PARAMETER1 root_param_vs;
                root_param_vs.InitAsConstants(vs_size / sizeof(uint32_t), vs_reflection->entryParametersBindingIndex, 0, D3D12_SHADER_VISIBILITY_VERTEX);
                parameters.push_back(root_param_vs);
            }

            uint32_t ps_size = 0;
            if (ps_reflection)
            {
                for (int i = 0; i < ps_reflection->entryPointParameters.size(); i++)
                {
                    ps_size += ps_reflection->entryPointParameters[i].size;
                }
                if (ps_size > 0)
                {
                    CD3DX12_ROOT_PARAMETER1 root_param_ps;
                    root_param_ps.InitAsConstants(ps_size / sizeof(uint32_t), ps_reflection->entryParametersBindingIndex, 0, D3D12_SHADER_VISIBILITY_PIXEL);
                    parameters.push_back(root_param_ps);
                }
            }
        }

        // All inline CBV's
        {
            uint64_t vs_cbv_mask = 0;
            for (int i = 0; i < vs_reflection->globalParameters.size(); i++)
            {
                if (vs_reflection->globalParameters[i].type == ParamType::kConstantBuffer)
                    vs_cbv_mask |= (1 << vs_reflection->globalParameters[i].bindingIndex);
            }
            uint64_t ps_cbv_mask = 0;
            if (ps_reflection)
            {
                for (int i = 0; i < ps_reflection->globalParameters.size(); i++)
                {
                    if (ps_reflection->globalParameters[i].type == ParamType::kConstantBuffer)
                        ps_cbv_mask |= (1 << ps_reflection->globalParameters[i].bindingIndex);
                }
            }

            uint64_t combined_cbv_mask = vs_cbv_mask | ps_cbv_mask;

            DWORD index;
            while (_BitScanForward(&index, combined_cbv_mask) && index < (sizeof(combined_cbv_mask) * 8))
            {
                CD3DX12_ROOT_PARAMETER1 root_param;

                if ((vs_cbv_mask & (1 << index)) > 0 && (ps_cbv_mask & (1 << index)) > 0)
                {
                    // Visible in both stages
                    root_param.InitAsConstantBufferView(index, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
                }
                else if ((vs_cbv_mask & (1 << index)) > 0)
                {
                    // Visible in vertex stage only
                    root_param.InitAsConstantBufferView(index, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
                }
                else
                {
                    // Visible in pixel stage only
                    root_param.InitAsConstantBufferView(index, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
                }

                parameters.push_back(root_param);

                // Flip the bit so it's not scanned again
                combined_cbv_mask ^= (1 << index);
            }
        }

        // It shouldn't matter if we pass in the vs/ps identifier here as samplers should be shared within the same Slang module
        List<D3D12_STATIC_SAMPLER_DESC> static_samplers = GetSamplerDescriptions(ss, vs_identifier);

        D3D12_ROOT_SIGNATURE_DESC1 desc = {};
        desc.Flags              = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.NumParameters      = parameters.size();
        desc.pParameters        = parameters.data();
        desc.NumStaticSamplers  = static_samplers.size();
        desc.pStaticSamplers    = static_samplers.data();

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_desc = {};
        versioned_desc.Version  = D3D_ROOT_SIGNATURE_VERSION_1_1;
        versioned_desc.Desc_1_1 = desc;

        GPtr<ID3DBlob> root_signature_blob;
        GPtr<ID3DBlob> error_blob;
        RB_ASSERT_FATAL_RELEASE_D3D(D3D12SerializeVersionedRootSignature(&versioned_desc, &root_signature_blob, &error_blob), "Could not serialize root signature");

        GPtr<ID3D12RootSignature> signature;
        RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&signature)), "Could not create root signature");

        m_RootSignatures.insert({ hash, signature });

        return signature;
    }

    GPtr<ID3D12RootSignature> PipelineManager::GetRootSignature(const ShaderSystem* ss, uint32_t cs_identifier)
    {
        uint64_t hash = 0;
        HashCombine(hash, cs_identifier);

        auto found = m_RootSignatures.find(hash);

        if (found != m_RootSignatures.end())
        {
            return found->second;
        }

        const auto* reflection = ss->GetReflection(cs_identifier);

        List<CD3DX12_ROOT_PARAMETER1> parameters;

        // Set entry point parameters as root constants
        {
            uint32_t size = 0;
            for (int i = 0; i < reflection->entryPointParameters.size(); i++)
            {
                size += reflection->entryPointParameters[i].size;
            }
            if (size > 0)
            {
                CD3DX12_ROOT_PARAMETER1 root_param;
                root_param.InitAsConstants(size / sizeof(uint32_t), reflection->entryParametersBindingIndex, 0, D3D12_SHADER_VISIBILITY_ALL);
                parameters.push_back(root_param);
            }
        }

        // All inline CBV's
        {
            for (int i = 0; i < reflection->globalParameters.size(); i++)
            {
                const GlobalParameter& param = reflection->globalParameters[i];
                if (param.type != ParamType::kConstantBuffer)
                {
                    continue;
                }

                CD3DX12_ROOT_PARAMETER1 root_param;
                root_param.InitAsConstantBufferView(param.bindingIndex, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
                parameters.push_back(root_param);
            }
        }

        List<D3D12_STATIC_SAMPLER_DESC> static_samplers = GetSamplerDescriptions(ss, cs_identifier);

        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

        D3D12_ROOT_SIGNATURE_DESC1 desc = {};
        desc.Flags              = flags;
        desc.NumParameters      = parameters.size();
        desc.pParameters        = parameters.data();
        desc.NumStaticSamplers  = static_samplers.size();
        desc.pStaticSamplers    = static_samplers.data();

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_desc = {};
        versioned_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        versioned_desc.Desc_1_1 = desc;

        GPtr<ID3DBlob> root_signature_blob;
        GPtr<ID3DBlob> error_blob;
        RB_ASSERT_FATAL_RELEASE_D3D(D3D12SerializeVersionedRootSignature(&versioned_desc, &root_signature_blob, &error_blob), "Could not serialize root signature");

        GPtr<ID3D12RootSignature> signature;
        RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&signature)), "Could not create root signature");

        m_RootSignatures.insert({ hash, signature });

        return signature;
    }

    uint64_t PipelineManager::GetPipelineHash(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, uint64_t root_signature_hash)
    {
        uint64_t seed = 0;

        HashCombine(seed, desc.NodeMask);
        HashCombine(seed, (char*)desc.CachedPSO.pCachedBlob);
        HashCombine(seed, (UINT)desc.Flags);
        HashCombine(seed, root_signature_hash);

        return seed;
    }

    uint64_t PipelineManager::GetPipelineHash(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint64_t root_signature_hash)
    {
        uint64_t seed = 0;

        HashCombine(seed, root_signature_hash);
        //HashCombine(seed, desc.StreamOutput);
        HashCombine(seed, desc.BlendState.AlphaToCoverageEnable);
        HashCombine(seed, desc.BlendState.IndependentBlendEnable);
        HashCombine(seed, desc.SampleMask);
        HashCombine(seed, (UINT)desc.RasterizerState.CullMode);
        HashCombine(seed, (UINT)desc.RasterizerState.FillMode);
        HashCombine(seed, desc.RasterizerState.FrontCounterClockwise);
        HashCombine(seed, desc.RasterizerState.DepthBias);
        HashCombine(seed, desc.RasterizerState.DepthClipEnable);
        HashCombine(seed, desc.DepthStencilState.DepthEnable);
        HashCombine(seed, (UINT)desc.DepthStencilState.DepthWriteMask);
        HashCombine(seed, (UINT)desc.DepthStencilState.DepthFunc);
        HashCombine(seed, desc.DepthStencilState.StencilEnable);
        HashCombine(seed, desc.DepthStencilState.StencilReadMask);
        HashCombine(seed, desc.DepthStencilState.StencilWriteMask);
        HashCombine(seed, (UINT)desc.DepthStencilState.FrontFace.StencilFailOp);
        HashCombine(seed, (UINT)desc.DepthStencilState.FrontFace.StencilDepthFailOp);
        HashCombine(seed, (UINT)desc.DepthStencilState.FrontFace.StencilPassOp);
        HashCombine(seed, (UINT)desc.DepthStencilState.FrontFace.StencilFunc);
        HashCombine(seed, (UINT)desc.DepthStencilState.BackFace.StencilFailOp);
        HashCombine(seed, (UINT)desc.DepthStencilState.BackFace.StencilDepthFailOp);
        HashCombine(seed, (UINT)desc.DepthStencilState.BackFace.StencilPassOp);
        HashCombine(seed, (UINT)desc.DepthStencilState.BackFace.StencilFunc);
        //HashCombine(seed, (UINT)desc.IBStripCutValue);
        HashCombine(seed, (UINT)desc.PrimitiveTopologyType);
        HashCombine(seed, desc.NumRenderTargets);
        HashCombine(seed, (UINT)desc.DSVFormat);
        //HashCombine(seed, desc.SampleDesc.Count);
        //HashCombine(seed, desc.SampleDesc.Quality);
        //HashCombine(seed, desc.NodeMask);
        HashCombine(seed, (UINT)desc.Flags);

        HashCombine(seed, desc.InputLayout.NumElements);
        for (int i = 0; i < desc.InputLayout.NumElements; ++i)
        {
            HashCombine(seed, desc.InputLayout.pInputElementDescs[i].InputSlot);
        }

        for (int i = 0; i < 8; ++i)
        {
            HashCombine(seed, desc.BlendState.RenderTarget[i].BlendEnable);
            HashCombine(seed, desc.BlendState.RenderTarget[i].LogicOpEnable);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].SrcBlend);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].DestBlend);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].BlendOp);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].SrcBlendAlpha);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].DestBlendAlpha);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].BlendOpAlpha);
            HashCombine(seed, (UINT)desc.BlendState.RenderTarget[i].LogicOp);
            HashCombine(seed, desc.BlendState.RenderTarget[i].RenderTargetWriteMask);
            HashCombine(seed, (UINT)desc.RTVFormats[i]);
        }

        return seed;
    }

    List<D3D12_STATIC_SAMPLER_DESC> PipelineManager::GetSamplerDescriptions(const ShaderSystem* ss, uint32_t shader_identifier)
    {
        List<D3D12_STATIC_SAMPLER_DESC> static_samplers;

        const auto* reflection = ss->GetReflection(shader_identifier);
        if (reflection == nullptr)
        {
            return static_samplers;
        }

        const auto& global_params = reflection->globalParameters;

        // Very ugly string compare loop :D
        for (int i = 0; i < global_params.size(); i++)
        {
            const GlobalParameter& param = global_params[i];
            if (param.type != ParamType::kSampler)
            {
                continue;
            }

            // TODO Set the filter (and max anisotropy) based on texture filter setting (maybe to do this we would need non static samplers)

            if (std::strstr(param.name.c_str(), "g_ClampAnisoSampler"))
            {
                D3D12_STATIC_SAMPLER_DESC clamp = {};
                clamp.ShaderRegister   = param.bindingIndex;
                clamp.RegisterSpace    = 0;
                clamp.Filter           = D3D12_FILTER_ANISOTROPIC;
                clamp.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp.MipLODBias       = 0;
                clamp.MaxAnisotropy    = 8;
                clamp.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                clamp.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                clamp.MinLOD           = 0.0f;
                clamp.MaxLOD           = D3D12_FLOAT32_MAX;
                clamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                static_samplers.push_back(clamp);
            }
            else if (std::strstr(param.name.c_str(), "g_ClampPointSampler"))
            {
                D3D12_STATIC_SAMPLER_DESC clamp_point = {};
                clamp_point.ShaderRegister   = param.bindingIndex;
                clamp_point.RegisterSpace    = 0;
                clamp_point.Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
                clamp_point.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_point.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_point.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_point.MipLODBias       = 0;
                clamp_point.MaxAnisotropy    = 1;
                clamp_point.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                clamp_point.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                clamp_point.MinLOD           = 0.0f;
                clamp_point.MaxLOD           = D3D12_FLOAT32_MAX;
                clamp_point.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                static_samplers.push_back(clamp_point);
            }
            else if (std::strstr(param.name.c_str(), "g_ClampLinearSampler"))
            {
                D3D12_STATIC_SAMPLER_DESC clamp_linear = {};
                clamp_linear.ShaderRegister   = param.bindingIndex;
                clamp_linear.RegisterSpace    = 0;
                clamp_linear.Filter           = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                clamp_linear.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_linear.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_linear.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_linear.MipLODBias       = 0;
                clamp_linear.MaxAnisotropy    = 1;
                clamp_linear.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                clamp_linear.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                clamp_linear.MinLOD           = 0.0f;
                clamp_linear.MaxLOD           = D3D12_FLOAT32_MAX;
                clamp_linear.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                static_samplers.push_back(clamp_linear);
            }
            else if (std::strstr(param.name.c_str(), "g_ClampLinearMipSampler"))
            {
                D3D12_STATIC_SAMPLER_DESC clamp_linear_mip = {};
                clamp_linear_mip.ShaderRegister   = param.bindingIndex;
                clamp_linear_mip.RegisterSpace    = 0;
                clamp_linear_mip.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                clamp_linear_mip.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_linear_mip.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_linear_mip.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                clamp_linear_mip.MipLODBias       = 0;
                clamp_linear_mip.MaxAnisotropy    = 1;
                clamp_linear_mip.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                clamp_linear_mip.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                clamp_linear_mip.MinLOD           = 0.0f;
                clamp_linear_mip.MaxLOD           = D3D12_FLOAT32_MAX;
                clamp_linear_mip.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                static_samplers.push_back(clamp_linear_mip);
            }
            else if (std::strstr(param.name.c_str(), "g_WrapAnisoSampler"))
            {
                D3D12_STATIC_SAMPLER_DESC wrap = {};
                wrap.ShaderRegister   = param.bindingIndex;
                wrap.RegisterSpace    = 0;
                wrap.Filter           = D3D12_FILTER_ANISOTROPIC;
                wrap.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                wrap.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                wrap.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                wrap.MipLODBias       = 0;
                wrap.MaxAnisotropy    = 8;
                wrap.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                wrap.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                wrap.MinLOD           = 0.0f;
                wrap.MaxLOD           = D3D12_FLOAT32_MAX;
                wrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                static_samplers.push_back(wrap);
            }
        }

        return static_samplers;
    }

    List<D3D12_INPUT_ELEMENT_DESC> PipelineManager::GetInputElementDesc(const ShaderSystem* ss, uint32_t vs_identifier, uint32_t vertex_buffers_count)
    {
        uint64_t hash = 0;
        HashCombine(hash, vs_identifier);
        HashCombine(hash, vertex_buffers_count);

        auto found = m_InputElementDescriptions.find(hash);

        if (found != m_InputElementDescriptions.end())
        {
            return found->second;
        }

        const auto& vertex_params = ss->GetReflection(vs_identifier)->vertexParameters;

        List<D3D12_INPUT_ELEMENT_DESC> elements(vertex_params.size());

        int32_t prev_input_slot = -1;
        uint32_t element_offset = 0;

        for (int i = 0; i < vertex_params.size(); ++i)
        {
            const VertexEntryParameter& param = vertex_params[i];

            uint32_t input_slot = Math::Min(uint32_t(prev_input_slot + 1), vertex_buffers_count - 1);
            if (input_slot != prev_input_slot)
            {
                element_offset = 0;
            }

            elements[i] = {};
            elements[i].SemanticName            = param.semantic.c_str();
            elements[i].SemanticIndex           = param.semanticIndex;
            elements[i].InstanceDataStepRate    = 0;
            elements[i].InputSlot               = input_slot;
            elements[i].AlignedByteOffset       = element_offset;
            elements[i].InputSlotClass          = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

            prev_input_slot = input_slot;

            switch (param.scalarType)
            {
            case ScalarType::UInt32:
            {
                element_offset += sizeof(uint32_t) * param.elements;

                switch (param.elements)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R32_UINT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R32G32_UINT; break;
                case 3: elements[i].Format = DXGI_FORMAT_R32G32B32_UINT; break;
                case 4:
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            case ScalarType::Int32:
            {
                element_offset += sizeof(int32_t) * param.elements;

                switch (param.elements)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R32_SINT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R32G32_SINT; break;
                case 3: elements[i].Format = DXGI_FORMAT_R32G32B32_SINT; break;
                case 4:
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            case ScalarType::Float32:
            {
                element_offset += sizeof(float) * param.elements;

                switch (param.elements)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R32_FLOAT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R32G32_FLOAT; break;
                case 3: elements[i].Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                case 4: elements[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            case ScalarType::UInt16:
            {
                element_offset += sizeof(uint16_t) * param.elements;

                switch (param.elements)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R16_UINT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R16G16_UINT; break;
                case 4: elements[i].Format = DXGI_FORMAT_R16G16B16A16_UINT; break;
                case 3:
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            case ScalarType::Int16:
            {
                element_offset += sizeof(int16_t) * param.elements;

                switch (param.elements)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R16_SINT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R16G16_SINT; break;
                case 4: elements[i].Format = DXGI_FORMAT_R16G16B16A16_SINT; break;
                case 3:
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            case ScalarType::Float16:
            {
                element_offset += sizeof(uint16_t) * param.elements;

                switch (param.elements)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R16_FLOAT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R16G16_FLOAT; break;
                case 4: elements[i].Format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
                case 3:
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            default:
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                return elements;
            }
        }

        m_InputElementDescriptions.emplace(hash, elements);

        return elements;
    }
}
#endif