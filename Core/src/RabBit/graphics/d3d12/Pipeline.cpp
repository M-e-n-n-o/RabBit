#include "RabBitCommon.h"
#include "Pipeline.h"
#include "ShaderSystem.h"
#include "GraphicsDevice.h"
#include "resource/Descriptor.h"
#include "graphics/shaders/shared/Common.h"

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

    GPtr<ID3D12PipelineState> PipelineManager::GetGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint32_t vs_identifier, uint32_t ps_identifier)
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

    GPtr<ID3D12RootSignature> PipelineManager::GetRootSignature(uint32_t vs_identifier, uint32_t ps_identifier)
    {
        uint64_t hash = 0;
        HashCombine(hash, vs_identifier);
        HashCombine(hash, ps_identifier);

        auto found = m_RootSignatures.find(hash);

        if (found != m_RootSignatures.end())
        {
            return found->second;
        }

        const ShaderResourceMask& vs_mask = g_ShaderSystem->GetShaderResourceMask(vs_identifier);
        const ShaderResourceMask& ps_mask = g_ShaderSystem->GetShaderResourceMask(ps_identifier);

        uint64_t combined_cbv_mask = vs_mask.cbvMask | ps_mask.cbvMask;

        // Num parameters:
        // - All inline CBV's
        uint32_t num_parameters = NumberOfSetBits(combined_cbv_mask);

        CD3DX12_ROOT_PARAMETER1* parameters = (CD3DX12_ROOT_PARAMETER1*)ALLOC_STACK(sizeof(CD3DX12_ROOT_PARAMETER1) * num_parameters);

        // Parameters
        {
            uint32_t parameter_index = 0;

            // Inline CBV's
            {
                RB_ASSERT(LOGTAG_GRAPHICS, parameter_index == CBV_ROOT_PARAMETER_INDEX_OFFSET, "These should match!");

                DWORD index;
                while (_BitScanForward(&index, combined_cbv_mask) && index < (sizeof(combined_cbv_mask) * 8))
                {
                    if ((vs_mask.cbvMask & (1 << index)) > 0 && (ps_mask.cbvMask & (1 << index)) > 0)
                    {
                        // Visible in both stages
                        parameters[parameter_index].InitAsConstantBufferView(index, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
                    }
                    else if ((vs_mask.cbvMask & (1 << index)) > 0)
                    {
                        // Visible in vertex stage only
                        parameters[parameter_index].InitAsConstantBufferView(index, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
                    }
                    else
                    {
                        // Visible in pixel stage only
                        parameters[parameter_index].InitAsConstantBufferView(index, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
                    }

                    // Flip the bit so it's not scanned again
                    combined_cbv_mask ^= (1 << index);

                    parameter_index++;
                }
            }
        }

        List<D3D12_STATIC_SAMPLER_DESC> static_samplers = GetSamplerDescriptions();

        D3D12_ROOT_SIGNATURE_DESC1 desc = {};
        desc.Flags              = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.NumParameters      = num_parameters;
        desc.pParameters        = parameters;
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

    GPtr<ID3D12RootSignature> PipelineManager::GetRootSignature(uint32_t cs_identifier)
    {
        uint64_t hash = 0;
        HashCombine(hash, cs_identifier);

        auto found = m_RootSignatures.find(hash);

        if (found != m_RootSignatures.end())
        {
            return found->second;
        }

        uint64_t cbv_mask = g_ShaderSystem->GetShaderResourceMask(cs_identifier).cbvMask;

        // Num parameters:
        // - All inline CBV's
        uint32_t num_parameters = NumberOfSetBits(cbv_mask);

        CD3DX12_ROOT_PARAMETER* parameters = (CD3DX12_ROOT_PARAMETER*)ALLOC_STACK(sizeof(CD3DX12_ROOT_PARAMETER) * num_parameters);

        // Parameters
        {
            uint32_t parameter_index = 0;

            // Inline CBV's
            {
                RB_ASSERT(LOGTAG_GRAPHICS, parameter_index == CBV_ROOT_PARAMETER_INDEX_OFFSET, "These should match!");

                DWORD index;
                while (_BitScanForward(&index, cbv_mask) && index < (sizeof(cbv_mask) * 8))
                {
                    if ((cbv_mask & (1 << index)) > 0)
                    {
                        parameters[parameter_index].InitAsConstantBufferView(index, 0, D3D12_SHADER_VISIBILITY_ALL);
                    }

                    // Flip the bit so it's not scanned again
                    cbv_mask ^= (1 << index);

                    parameter_index++;
                }
            }
        }

        List<D3D12_STATIC_SAMPLER_DESC> static_samplers = GetSamplerDescriptions();

        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.Flags              = flags;
        desc.NumParameters      = num_parameters;
        desc.pParameters        = parameters;
        desc.NumStaticSamplers  = static_samplers.size();
        desc.pStaticSamplers    = static_samplers.data();

        GPtr<ID3DBlob> root_signature_blob;
        GPtr<ID3DBlob> error_blob;
        RB_ASSERT_FATAL_RELEASE_D3D(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob), "Could not serialize root signature");

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
        HashCombine(seed, (UINT)desc.IBStripCutValue);
        HashCombine(seed, (UINT)desc.PrimitiveTopologyType);
        HashCombine(seed, desc.NumRenderTargets);
        HashCombine(seed, (UINT)desc.DSVFormat);
        HashCombine(seed, desc.SampleDesc.Count);
        HashCombine(seed, desc.SampleDesc.Quality);
        HashCombine(seed, desc.NodeMask);
        HashCombine(seed, (UINT)desc.Flags);

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

    List<D3D12_STATIC_SAMPLER_DESC> PipelineManager::GetSamplerDescriptions()
    {
        List<D3D12_STATIC_SAMPLER_DESC> static_samplers;

        // TODO Set the filter (and max anisotropy) based on texture filter setting (maybe to do this we would need non static samplers)

        // Clamp sampler
        D3D12_STATIC_SAMPLER_DESC clamp = {};
        clamp.ShaderRegister   = kClampAnisoSamplerSlot;
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

        // Black border sampler
        D3D12_STATIC_SAMPLER_DESC clamp_point = {};
        clamp_point.ShaderRegister   = kClampPointSamplerSlot;
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

        return static_samplers;
    }

    List<D3D12_INPUT_ELEMENT_DESC> PipelineManager::GetInputElementDesc(uint32_t vs_identifier)
    {
        auto found = m_InputElementDescriptions.find(vs_identifier);

        if (found != m_InputElementDescriptions.end())
        {
            return found->second;
        }

        GPtr<ID3D12ShaderReflection> reflection = g_ShaderSystem->GetCompilerShader(vs_identifier)->reflectionData;

        D3D12_SHADER_DESC shader_desc;
        reflection->GetDesc(&shader_desc);

        List<D3D12_INPUT_ELEMENT_DESC> elements(shader_desc.InputParameters);

        uint32_t element_offset = 0;

        for (int i = 0; i < shader_desc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC desc;
            reflection->GetInputParameterDesc(i, &desc);

            elements[i] = {};
            elements[i].SemanticName = desc.SemanticName;
            elements[i].SemanticIndex = desc.SemanticIndex;
            elements[i].InstanceDataStepRate = 0;
            // For now hardcode all VAO's to have interleaved data, TODO this should be made more flexible later
            elements[i].InputSlot = 0;
            elements[i].AlignedByteOffset = element_offset;
            elements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

            uint32_t channel_count = NumberOfSetBits(desc.Mask);

            switch (desc.ComponentType)
            {
            case D3D_REGISTER_COMPONENT_UINT32:
            {
                element_offset += sizeof(uint32_t) * channel_count;

                switch (channel_count)
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
            case D3D_REGISTER_COMPONENT_SINT32:
            {
                element_offset += sizeof(uint32_t) * channel_count;

                switch (channel_count)
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
            case D3D_REGISTER_COMPONENT_FLOAT32:
            {
                element_offset += sizeof(float) * channel_count;

                switch (channel_count)
                {
                case 1: elements[i].Format = DXGI_FORMAT_R32_FLOAT; break;
                case 2: elements[i].Format = DXGI_FORMAT_R32G32_FLOAT; break;
                case 3: elements[i].Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                case 4:
                default:
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                    return elements;
                }
            }
            break;
            case D3D_REGISTER_COMPONENT_UINT16:
            {
                element_offset += sizeof(uint16_t) * channel_count;

                switch (channel_count)
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
            case D3D_REGISTER_COMPONENT_SINT16:
            {
                element_offset += sizeof(uint16_t) * channel_count;

                switch (channel_count)
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
            case D3D_REGISTER_COMPONENT_FLOAT16:
            {
                element_offset += sizeof(uint16_t) * channel_count;

                switch (channel_count)
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
            case D3D_REGISTER_COMPONENT_UINT64:
            case D3D_REGISTER_COMPONENT_SINT64:
            case D3D_REGISTER_COMPONENT_FLOAT64:
            default:
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Format not recognized");
                return elements;
            }
        }

        m_InputElementDescriptions.insert({ vs_identifier, elements });

        return elements;
    }
}