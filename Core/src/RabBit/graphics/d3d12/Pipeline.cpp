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
		const ShaderResourceMask&	 vs_mask	   = m_ShaderSystem->GetShaderResourceMask(vs_identifier);
		const ShaderResourceMask&	 ps_mask	   = m_ShaderSystem->GetShaderResourceMask(ps_identifier);

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

		//uint32_t referenced_cbvs = vs_desc.ConstantBuffers + ps_desc.ConstantBuffers;

		//// Only CBV's are supported currently.
		//// All constant buffers are inline for now.
		//uint32_t total_cbvs = 0;
		//D3D12_ROOT_PARAMETER* parameters	  = (D3D12_ROOT_PARAMETER*) ALLOC_STACK(sizeof(D3D12_ROOT_PARAMETER) * referenced_cbvs);
		//LPCSTR*				  parameter_names = (LPCSTR*)				ALLOC_STACK(sizeof(LPCSTR) * referenced_cbvs);

		//for (int i = 0; i < referenced_cbvs; ++i)
		//{
		//	GPtr<ID3D12ShaderReflection>	reflection	= i < vs_desc.ConstantBuffers ? vs_reflection : ps_reflection;
		//	uint32_t						subtract	= i < vs_desc.ConstantBuffers ? 0 : vs_desc.ConstantBuffers;
		//	D3D12_SHADER_VISIBILITY			visibility	= i < vs_desc.ConstantBuffers ? D3D12_SHADER_VISIBILITY_VERTEX : D3D12_SHADER_VISIBILITY_PIXEL;

		//	D3D12_SHADER_BUFFER_DESC buffer_desc;
		//	reflection->GetConstantBufferByIndex(i - subtract)->GetDesc(&buffer_desc);

		//	int32_t index = -1;
		//	for (int j = 0; j < total_cbvs; ++j)
		//	{
		//		if (parameter_names[j] == buffer_desc.Name)
		//		{
		//			// Found the same CBV (so it's shared between multiple shader stages)
		//			index = j;
		//			break;
		//		}
		//	}

		//	if (index >= 0)
		//	{
		//		parameters[index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//	}
		//	else
		//	{
		//		parameters[total_cbvs] = {};
		//		parameters[total_cbvs].ParameterType			 = D3D12_ROOT_PARAMETER_TYPE_CBV;
		//		parameters[total_cbvs].Descriptor.ShaderRegister = total_cbvs;
		//		parameters[total_cbvs].Descriptor.RegisterSpace	 = 0;
		//		parameters[total_cbvs].ShaderVisibility			 = visibility;

		//		parameter_names[total_cbvs] = buffer_desc.Name;

		//		total_cbvs++;
		//	}
		//}

		//uint32_t total_slots = NumberOfSetBits(vs_mask.cbvMask) + NumberOfSetBits(vs_mask.srvMask) + NumberOfSetBits(vs_mask.uavMask) + NumberOfSetBits(vs_mask.samplerMask);
		//						+ NumberOfSetBits(ps_mask.cbvMask) + NumberOfSetBits(ps_mask.srvMask) + NumberOfSetBits(ps_mask.uavMask) + NumberOfSetBits(ps_mask.samplerMask);



		CD3DX12_DESCRIPTOR_RANGE vertex_visible_sampler_ranges;
		CD3DX12_DESCRIPTOR_RANGE pixel_visible_sampler_ranges;
		CD3DX12_DESCRIPTOR_RANGE all_visible_sampler_ranges;



		uint32_t num_parameters = 0;

		// Samplers will have their own descriptor table
		CD3DX12_DESCRIPTOR_RANGE sampler_range;
		uint32_t combined_samplers = vs_mask.samplerMask | ps_mask.samplerMask;
		uint32_t total_samplers = NumberOfSetBits(combined_samplers);

		if (total_samplers > 0)
		{
			DWORD index;
			_BitScanForward(&index, combined_samplers);

			sampler_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, total_samplers, index);

			num_parameters++;
		}

		// SRV's and UAV's will be combined into 1 descriptor table
		CD3DX12_DESCRIPTOR_RANGE srv_uav_ranges[2];
		uint32_t total_srv_uav_ranges = 0;

		// SRV's
		CD3DX12_DESCRIPTOR_RANGE srv_range;
		uint32_t combined_srvs = vs_mask.srvMask | ps_mask.srvMask;
		uint32_t total_srvs = NumberOfSetBits(combined_srvs);

		if (total_srvs > 0)
		{
			DWORD index;
			_BitScanForward(&index, combined_srvs);

			srv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, total_srvs, index);

			srv_uav_ranges[total_srv_uav_ranges] = srv_range;
			total_srv_uav_ranges++;
		}

		// UAV's
		CD3DX12_DESCRIPTOR_RANGE uav_range;
		uint32_t combined_uavs = vs_mask.uavMask | ps_mask.uavMask;
		uint32_t total_uavs = NumberOfSetBits(combined_uavs);

		if (total_uavs > 0)
		{
			DWORD index;
			_BitScanForward(&index, combined_uavs);

			uav_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, total_uavs, index);

			srv_uav_ranges[total_srv_uav_ranges] = uav_range;
			total_srv_uav_ranges++;
		}

		num_parameters = total_srv_uav_ranges > 0 ? num_parameters + 1 : num_parameters;

		// CBV's are all inline
		uint32_t combined_cbvs = vs_mask.cbvMask | ps_mask.cbvMask;
		uint32_t total_cbvs = NumberOfSetBits(combined_cbvs);

		num_parameters += total_cbvs;

		CD3DX12_ROOT_PARAMETER* parameters = (CD3DX12_ROOT_PARAMETER*)ALLOC_STACK(sizeof(CD3DX12_ROOT_PARAMETER) * num_parameters);

		// TODO How do we properly set the visibility???????
		// We need to divide the ranges into which are visible in which shader stage!!
		//D3D12_SHADER_VISIBILITY_VERTEX
		//parameters[0].InitAsDescriptorTable();
		//parameters[0].InitAsConstantBufferView()


		for (int i = 0; i < 2; ++i)
		{
			const ShaderResourceMask& mask = i == 0 ? vs_mask : ps_mask;

			uint64_t cbv_mask = mask.cbvMask;
			DWORD index;

			while (_BitScanForward(&index, cbv_mask) && index < NumberOfSetBits(mask.cbvMask))
			{
				parameters

				// Flip the bit so it's not scanned again
				cbv_mask ^= (1 << index);
			}
		}

		// GO BINDLESS FOR SRV's, and CBV's (maybe also samplers?)!!!!!!
		// https://alextardif.com/Bindless.html
		// https://blog.traverseresearch.nl/bindless-rendering-setup-afeb678d77fc
		// https://rtarun9.github.io/blogs/bindless_rendering/
		/*
			Plan:
			(Only do bindless for SRV's & UAV's, use static samplers & inline CBV's)
			- Create 1 shader visisble CBV_SRV_UAV descriptor heap with the max amount of descriptors possible
			- Create 1 non-shader visisble CBV_SRV_UAV heap with space for only 1 descriptor
			- Bind the descriptor heap directly at the start of each command list (so not needed after every call of pso change anymore)
			- Create separate descriptor ranges (in the root signature) for each resource type (tex2D, cubeTex, tex3D, rwTex2D etc.) with the max amount of descriptors possible 
				(https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support) (do we also need the D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE & D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE flags??)
			- When a texture is created, a SRV (and UAV if write access) is created in the first empty spot on the descriptor heap (so directly copied into the GPU descriptor heap, 
			    so cpu descriptor does not need to be kept around). The view index into the heap is stored in the texture class.
			- When you then want to set a texture in a shader you still need to specify which slot you are going to use in the TextureIndices constant buffer.
			- The RenderInterface will then upload the index of the view into the shader visible heap into that slot so the shader knows which texture to pick from the texture array.


			Example implementation:

			GPU:
			{
				Texture2D   Texture2DTable[]   : register(t0, tex2DSpace);
				TextureCube TextureCubeTable[] : registre(t0, texCubeSpace);
				RWTexture2D RwTexture2DTable[] : register(t0, rwTex2DSpace);

				// TODO: Maybe make this a Buffer (or ByteAddressBuffer) instead of an inline CBV
				cbuffer TextureIndices : CBUFFER_REG(kInstanceCB)
				{
					uint g_IndexTex2D0; // The indices that are not used should point to a default error texture!
					uint g_IndexTex2D1;
					uint g_IndexTex2D2;
					etc.

					uint g_IndexTexCube0;
					uint g_IndexTexCube1;
					uint g_IndexTexCube2;
					etc.

					etc.
				}

				#define FETCH_TEX2D(index) Texture2DTable[g_IndexTex2D##index##]

				main()
				{
					Texture2D<float4> albedo_tex = FETCH_TEX2D(1);
				}
			}

			CPU:
			{
				interface->SetSRV(1, albedo_tex);



				RenderInterface::SetSRV(slot, texture)
				{
					tex_slot = DescriptorHeap::GetSRVIndex(texture);

					// MAKE SURE THE CONSTANT BUFFER IS DOUBLE BUFFERED (just use the already existing SetConstantShaderData method)
					shader->uploadConstant("g_IndexTex2D{slot}", tex_slot);
				}
			}
		
		*/
		static_assert(false);


		// TODO Expand the creation of root signatures based on the shader reflection data!
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		desc.NumParameters		= num_parameters;
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

	void PipelineManager::FillDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t vs_mask, uint32_t ps_mask, 
		CD3DX12_DESCRIPTOR_RANGE* out_vertex_range, CD3DX12_DESCRIPTOR_RANGE* out_pixel_range, CD3DX12_DESCRIPTOR_RANGE* out_all_range)
	{
		uint32_t combined_mask = vs_mask | ps_mask;

		DWORD index;
		while (_BitScanForward(&index, combined_mask) && index < NumberOfSetBits(combined_mask))
		{
			if ((vs_mask & (1 << index)) > 0 && (ps_mask & (1 << index)) > 0)
			{
				// Visible in both stages
				out_all_range->Init();
			}
			else if ((vs_mask & (1 << index)) > 0)
			{
				// Visible in vertex stage only
			}
			else
			{
				// Visible in pixel stage only
			}

			// Flip the bit so it's not scanned again
			combined_mask ^= (1 << index);
		}
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