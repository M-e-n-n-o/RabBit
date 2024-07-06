#include "RabBitCommon.h"
#include "Pipeline.h"
#include "graphics/d3d12/GraphicsDevice.h"
#include "graphics/d3d12/shaders/ShaderSystemD3D12.h"

namespace RB::Graphics::D3D12
{
	PipelineManager* g_PipelineManager = nullptr;

	PipelineManager::PipelineManager(ShaderSystem* shader_system)
		: m_ShaderSystem(shader_system)
	{

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

		GPtr<ID3D12ShaderReflection> vs_reflection = ((CompiledShaderBlob*) m_ShaderSystem->GetCompilerShader(vs_identifier))->reflectionData;
		GPtr<ID3D12ShaderReflection> ps_reflection = ((CompiledShaderBlob*)m_ShaderSystem->GetCompilerShader(ps_identifier))->reflectionData;

		D3D12_SHADER_DESC vs_desc;
		vs_reflection->GetDesc(&vs_desc);
		D3D12_SHADER_DESC ps_desc;
		ps_reflection->GetDesc(&ps_desc);


		// Change all this super ugly logic to a bit mask system like that from:
		// https://github.com/jpvanoosten/LearningDirectX12/blob/v0.0.3/DX12Lib/src/RootSignature.cpp
		//
		// We can then also use this bit mask in the DynamicGpuDescriptorHeap class
		// FILL IN THE RootSignatureParameterDescriptorDesc STRUCT!!!
		// Also make sure that there cannot be more than 32 number of parameters in the root signature (rootIndexSlotsUsed)!
		static_assert(false);

		uint32_t referenced_cbvs = vs_desc.ConstantBuffers + ps_desc.ConstantBuffers;

		// Only CBV's are supported currently.
		// All constant buffers are inline for now.
		uint32_t total_cbvs = 0;
		D3D12_ROOT_PARAMETER* parameters	  = (D3D12_ROOT_PARAMETER*) ALLOC_STACK(sizeof(D3D12_ROOT_PARAMETER) * referenced_cbvs);
		LPCSTR*				  parameter_names = (LPCSTR*)				ALLOC_STACK(sizeof(LPCSTR) * referenced_cbvs);

		for (int i = 0; i < referenced_cbvs; ++i)
		{
			GPtr<ID3D12ShaderReflection>	reflection	= i < vs_desc.ConstantBuffers ? vs_reflection : ps_reflection;
			uint32_t						subtract	= i < vs_desc.ConstantBuffers ? 0 : vs_desc.ConstantBuffers;
			D3D12_SHADER_VISIBILITY			visibility	= i < vs_desc.ConstantBuffers ? D3D12_SHADER_VISIBILITY_VERTEX : D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_SHADER_BUFFER_DESC buffer_desc;
			reflection->GetConstantBufferByIndex(i - subtract)->GetDesc(&buffer_desc);

			int32_t index = -1;
			for (int j = 0; j < total_cbvs; ++j)
			{
				if (parameter_names[j] == buffer_desc.Name)
				{
					// Found the same CBV (so it's shared between multiple shader stages)
					index = j;
					break;
				}
			}

			if (index >= 0)
			{
				parameters[index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			else
			{
				parameters[total_cbvs] = {};
				parameters[total_cbvs].ParameterType			 = D3D12_ROOT_PARAMETER_TYPE_CBV;
				parameters[total_cbvs].Descriptor.ShaderRegister = total_cbvs;
				parameters[total_cbvs].Descriptor.RegisterSpace	 = 0;
				parameters[total_cbvs].ShaderVisibility			 = visibility;

				parameter_names[total_cbvs] = buffer_desc.Name;

				total_cbvs++;
			}
		}

		// TODO Expand the creation of root signatures based on the shader reflection data!
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		desc.NumParameters		= total_cbvs;
		desc.pParameters		= parameters;
		desc.NumStaticSamplers	= 0;
		desc.pStaticSamplers	= nullptr;

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
		HashCombine(seed, (char*) desc.CachedPSO.pCachedBlob);
		HashCombine(seed, (UINT) desc.Flags);
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
		HashCombine(seed, (UINT) desc.RasterizerState.CullMode);
		HashCombine(seed, (UINT) desc.RasterizerState.FillMode);
		HashCombine(seed, desc.RasterizerState.FrontCounterClockwise);
		HashCombine(seed, desc.RasterizerState.DepthBias);
		HashCombine(seed, desc.RasterizerState.DepthClipEnable);
		HashCombine(seed, desc.DepthStencilState.DepthEnable);
		HashCombine(seed, (UINT) desc.DepthStencilState.DepthWriteMask);
		HashCombine(seed, (UINT) desc.DepthStencilState.DepthFunc);
		HashCombine(seed, desc.DepthStencilState.StencilEnable);
		HashCombine(seed, desc.DepthStencilState.StencilReadMask);
		HashCombine(seed, desc.DepthStencilState.StencilWriteMask);
		HashCombine(seed, (UINT) desc.DepthStencilState.FrontFace.StencilFailOp);
		HashCombine(seed, (UINT) desc.DepthStencilState.FrontFace.StencilDepthFailOp);
		HashCombine(seed, (UINT) desc.DepthStencilState.FrontFace.StencilPassOp);
		HashCombine(seed, (UINT) desc.DepthStencilState.FrontFace.StencilFunc);
		HashCombine(seed, (UINT) desc.DepthStencilState.BackFace.StencilFailOp);
		HashCombine(seed, (UINT) desc.DepthStencilState.BackFace.StencilDepthFailOp);
		HashCombine(seed, (UINT) desc.DepthStencilState.BackFace.StencilPassOp);
		HashCombine(seed, (UINT) desc.DepthStencilState.BackFace.StencilFunc);
		HashCombine(seed, (UINT) desc.IBStripCutValue);
		HashCombine(seed, (UINT) desc.PrimitiveTopologyType);
		HashCombine(seed, desc.NumRenderTargets);
		HashCombine(seed, (UINT) desc.DSVFormat);
		HashCombine(seed, desc.SampleDesc.Count);
		HashCombine(seed, desc.SampleDesc.Quality);
		HashCombine(seed, desc.NodeMask);
		HashCombine(seed, (UINT) desc.Flags);

		for (int i = 0; i < 8; ++i)
		{
			HashCombine(seed, (UINT)desc.RTVFormats[i]);
		}

		return seed;
	}

	List<D3D12_INPUT_ELEMENT_DESC> PipelineManager::GetInputElementDesc(uint32_t vs_identifier)
	{
		auto found = m_InputElementDescriptions.find(vs_identifier);

		if (found != m_InputElementDescriptions.end())
		{
			return found->second;
		}

		GPtr<ID3D12ShaderReflection> reflection = ((CompiledShaderBlob*)m_ShaderSystem->GetCompilerShader(vs_identifier))->reflectionData;

		D3D12_SHADER_DESC shader_desc;
		reflection->GetDesc(&shader_desc);

		List<D3D12_INPUT_ELEMENT_DESC> elements(shader_desc.InputParameters);

		uint32_t element_offset = 0;

		for (int i = 0; i < shader_desc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC desc;
			reflection->GetInputParameterDesc(i, &desc);

			elements[i] = {};
			elements[i].SemanticName		 = desc.SemanticName;
			elements[i].SemanticIndex		 = desc.SemanticIndex;
			elements[i].InstanceDataStepRate = 0;
			// For now hardcode all VAO's to have interleaved data, TODO this should be made more flexible later
			elements[i].InputSlot			 = 0;
			elements[i].AlignedByteOffset	 = element_offset;
			elements[i].InputSlotClass		 = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

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