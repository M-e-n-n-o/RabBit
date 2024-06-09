#include "RabBitCommon.h"
#include "RenderInterfaceD3D12.h"
#include "DeviceQueue.h"
#include "resource/ResourceManager.h"
#include "resource/RenderResourceD3D12.h"
#include "resource/ResourceStateManager.h"
#include "resource/UploadAllocator.h"
#include "pipeline/Pipeline.h"
#include "shaders/ShaderSystem.h"
#include "UtilsD3D12.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	GpuGuardD3D12::GpuGuardD3D12(uint64_t fence_value, DeviceQueue* queue)
		: m_FenceValue(fence_value)
		, m_Queue(queue)
	{
	}

	bool GpuGuardD3D12::IsFinishedRendering()
	{
		return m_Queue->IsFenceReached(m_FenceValue);
	}

	void GpuGuardD3D12::WaitUntilFinishedRendering()
	{
		m_Queue->CpuWaitForFenceValue(m_FenceValue);
	}

	// ---------------------------------------------------------------------------
	//								GpuGuard
	// ---------------------------------------------------------------------------

	RenderInterfaceD3D12::RenderInterfaceD3D12(bool allow_only_copy_operations)
		: m_CopyOperationsOnly(allow_only_copy_operations)
		, m_RenderState()
		, m_CurrentCBVAllocator(nullptr)
	{
		if (allow_only_copy_operations)
			m_Queue = g_GraphicsDevice->GetCopyQueue();
		else
			m_Queue = g_GraphicsDevice->GetGraphicsQueue();

		SetNewCommandList();
	}

	RenderInterfaceD3D12::~RenderInterfaceD3D12()
	{
		SAFE_DELETE(m_CurrentCBVAllocator);

		while (!m_AvailableCBVAllocators.empty())
		{
			SAFE_DELETE(m_AvailableCBVAllocators.front());
			m_AvailableCBVAllocators.pop();
		}

		for (int i = 0; i < m_InFlightCBVAllocators.size(); ++i)
		{
			SAFE_DELETE(m_InFlightCBVAllocators[i].allocator);
		}
	}

	void RenderInterfaceD3D12::InvalidateState()
	{
		m_RenderState = {};
	}

	Shared<GpuGuard> RenderInterfaceD3D12::ExecuteInternal()
	{
		// TODO Maybe do the ExecuteCommandLists on a separate thread in the future?
		uint64_t fence_value = m_Queue->ExecuteCommandList(m_CommandList);

		g_ResourceManager->OnCommandListExecute(m_Queue, fence_value);

		if (m_CurrentCBVAllocator)
		{
			m_InFlightCBVAllocators.push_back({ m_CurrentCBVAllocator, fence_value });

			m_CurrentCBVAllocator = nullptr;
		}

		InvalidateState();
		SetNewCommandList();

		return CreateShared<GpuGuardD3D12>(fence_value, m_Queue);
	}

	void RenderInterfaceD3D12::GpuWaitOn(GpuGuard* guard)
	{
		GpuGuardD3D12* d3d_guard = (GpuGuardD3D12*) guard;
		m_Queue->GpuWaitForFenceValue(d3d_guard->m_Queue->GetFence(), d3d_guard->m_FenceValue);
	}

	void RenderInterfaceD3D12::TransitionResource(RenderResource* resource, ResourceState state)
	{
		g_ResourceStateManager->TransitionResource((GpuResource*)resource->GetNativeResource(), ConvertToD3D12ResourceState(state));
	}

	void RenderInterfaceD3D12::FlushResourceBarriers()
	{
		g_ResourceStateManager->FlushPendingTransitions(m_CommandList.Get());
	}

	void RenderInterfaceD3D12::SetRenderTarget(RenderTargetBundle* bundle)
	{
		uint32_t max_width = 0;
		uint32_t max_height = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE color_handles[8];
		
		for (int i = 0; i < 8; ++i)
		{
			if (i < bundle->colorTargetsCount)
			{
				Texture2DD3D12* tex = (Texture2DD3D12*) bundle->colorTargets[i];
				color_handles[i] = *tex->GetCpuHandle();

				max_width = Math::Max(max_width, tex->GetWidth());
				max_height = Math::Max(max_height, tex->GetHeight());

				g_ResourceStateManager->TransitionResource((GpuResource*) tex->GetNativeResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

				m_RenderState.rtvFormats[i] = ConvertToDXGIFormat(tex->GetFormat());
			}
			else
			{
				m_RenderState.rtvFormats[i] = DXGI_FORMAT_UNKNOWN;
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE* depth_handle = nullptr;

		Texture2D* depth_stencil = bundle->depthStencilTarget;
		if (depth_stencil)
		{
			depth_handle = ((Texture2DD3D12*)depth_stencil)->GetCpuHandle();

			g_ResourceStateManager->TransitionResource((GpuResource*) depth_stencil->GetNativeResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

			m_RenderState.dsvFormat = ConvertToDXGIFormat(depth_stencil->GetFormat());
		}
		
		m_CommandList->OMSetRenderTargets(bundle->colorTargetsCount, color_handles, true, depth_handle);

		m_RenderState.numRenderTargets = bundle->colorTargetsCount;

		// Also set the scissor and rect
		SetScissorRect(0, LONG_MAX, 0, LONG_MAX);
		SetViewport(0, 0, max_width, max_height);

		m_RenderState.psoDirty = true;
	}

	void RenderInterfaceD3D12::SetConstantShaderData(uint32_t slot, void* data, uint32_t data_size)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, slot < _countof(m_RenderState.cbvAddresses), "Up the amount of possible CBV addresses");

		if (m_CurrentCBVAllocator == nullptr)
		{
			// Update the available allocators
			{
				auto itr = m_InFlightCBVAllocators.begin();
				while (itr != m_InFlightCBVAllocators.end())
				{
					if (m_Queue->IsFenceReached(itr->fenceValue))
					{
						m_AvailableCBVAllocators.push(itr->allocator);
						itr = m_InFlightCBVAllocators.erase(itr);
					}
					else
					{
						++itr;
					}
				}
			}

			if (m_AvailableCBVAllocators.empty())
			{
				m_CurrentCBVAllocator = new UploadAllocator("CBV Upload Allocator", k64KB);
			}
			else
			{
				m_CurrentCBVAllocator = m_AvailableCBVAllocators.front();
				m_AvailableCBVAllocators.pop();
			}
		}

		UploadAllocation allocation = m_CurrentCBVAllocator->Allocate(data_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		memcpy(allocation.cpuWriteAddress, data, data_size);

		m_RenderState.cbvAddresses[slot] = allocation.address;
	}

	void RenderInterfaceD3D12::SetVertexShader(uint32_t shader_index)
	{
		m_RenderState.vsShader = shader_index;
		m_RenderState.psoDirty = true;
		m_RenderState.rootSignatureDirty = true;
	}

	void RenderInterfaceD3D12::SetPixelShader(uint32_t shader_index)
	{
		m_RenderState.psShader = shader_index;
		m_RenderState.psoDirty = true;
		m_RenderState.rootSignatureDirty = true;
	}

	void RenderInterfaceD3D12::SetNewCommandList()
	{
		m_CommandList = m_Queue->GetCommandList();
	}

	void RenderInterfaceD3D12::Clear(RenderResource* resource, const Math::Float4& color)
	{
		g_ResourceStateManager->TransitionResource((GpuResource*)resource->GetNativeResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
		FlushResourceBarriers();

		FLOAT clear_color[] = { color.r, color.g, color.b, color.a };
		m_CommandList->ClearRenderTargetView(*((D3D12::Texture2DD3D12*)resource)->GetCpuHandle(), clear_color, 0, nullptr);
	}

	void RenderInterfaceD3D12::SetScissorRect(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
	{
		D3D12_RECT rect = {};
		rect.left	= left;
		rect.right	= right;
		rect.top	= top;
		rect.bottom = bottom;

		m_CommandList->RSSetScissorRects(1, &rect);

		m_RenderState.scissorSet = true;

		m_RenderState.psoDirty = true;
	}

	void RenderInterfaceD3D12::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX	= x;
		viewport.TopLeftY	= y;
		viewport.Width		= width;
		viewport.Height		= height;
		viewport.MinDepth	= D3D12_MIN_DEPTH;
		viewport.MaxDepth	= D3D12_MAX_DEPTH;

		m_CommandList->RSSetViewports(1, &viewport);

		m_RenderState.viewportSet = true;

		m_RenderState.psoDirty = true;
	}

	void RenderInterfaceD3D12::SetBlendMode(const BlendMode& mode)
	{
		D3D12_BLEND_DESC desc = {};

		switch (mode)
		{
		case BlendMode::None:
		{
			desc.AlphaToCoverageEnable						= false;
			desc.IndependentBlendEnable						= false;

			for (int i = 0; i < _countof(desc.RenderTarget); ++i)
			{
				desc.RenderTarget[i].BlendEnable			= false;
				desc.RenderTarget[i].LogicOpEnable			= false;
				desc.RenderTarget[i].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;
			}
		}
		break;

		default:
			RB_LOG_WARN(LOGTAG_GRAPHICS, "Did not yet implement this blend mode for the RenderInterfaceD3D12");
			return;
		}

		m_RenderState.blendDesc = desc;
		m_RenderState.blendingSet = true;

		m_RenderState.psoDirty = true;
	}

	void RenderInterfaceD3D12::SetCullMode(const CullMode& mode)
	{
		D3D12_RASTERIZER_DESC desc = {};
		desc.FillMode				= D3D12_FILL_MODE_SOLID;
		desc.DepthClipEnable		= TRUE;
		desc.FrontCounterClockwise	= FALSE;
		desc.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
		desc.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.SlopeScaledDepthBias	= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.MultisampleEnable		= FALSE;
		desc.AntialiasedLineEnable	= FALSE;
		desc.ForcedSampleCount		= 0;
		desc.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		switch (mode)
		{
		case CullMode::None:  desc.CullMode = D3D12_CULL_MODE_NONE;  break;
		case CullMode::Front: desc.CullMode = D3D12_CULL_MODE_FRONT; break;
		case CullMode::Back:  desc.CullMode = D3D12_CULL_MODE_BACK;  break;

		default:
			RB_LOG_WARN(LOGTAG_GRAPHICS, "Did not yet implement this cull mode for the RenderInterfaceD3D12");
			return;
		}

		m_RenderState.rasterizerSet = true;
		m_RenderState.rasterizerDesc = desc;

		m_RenderState.psoDirty = true;
	}

	void RenderInterfaceD3D12::SetDepthMode(const DepthMode& mode)
	{
		D3D12_DEPTH_STENCIL_DESC desc = {};
		desc.StencilEnable					= FALSE;
		desc.StencilReadMask				= 0;
		desc.StencilWriteMask				= 0;
		//desc.FrontFace.StencilFailOp		= ;
		//desc.FrontFace.StencilDepthFailOp	= ;
		//desc.FrontFace.StencilPassOp		= ;
		//desc.FrontFace.StencilFunc		= ;
		//desc.BackFace.StencilFailOp		= ;
		//desc.BackFace.StencilDepthFailOp	= ;
		//desc.BackFace.StencilPassOp		= ;
		//desc.BackFace.StencilFunc			= ;

		switch (mode)
		{
		case DepthMode::Disabled:
		{
			desc.DepthEnable	= FALSE;
			desc.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc		= D3D12_COMPARISON_FUNC_ALWAYS;
		}
		break;

		default:
			RB_LOG_WARN(LOGTAG_GRAPHICS, "Did not yet implement this depth mode for the RenderInterfaceD3D12");
			return;
		}

		m_RenderState.depthStencilSet = true;
		m_RenderState.depthStencilDesc = desc;

		m_RenderState.psoDirty = true;
	}

	void RenderInterfaceD3D12::SetIndexBuffer(RenderResource* index_resource)
	{
		IndexBufferD3D12* ib = (IndexBufferD3D12*) index_resource;

		m_CommandList->IASetIndexBuffer(&ib->GetView());

		m_RenderState.indexCountPerInstance = ib->GetIndexCount();
	}

	void RenderInterfaceD3D12::SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot)
	{
		RenderResource* resources[] = { vertex_resource };
		SetVertexBuffers(resources, 1, slot);
	}

	void RenderInterfaceD3D12::SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot)
	{
		D3D12_VERTEX_BUFFER_VIEW* views = (D3D12_VERTEX_BUFFER_VIEW*)ALLOC_STACK(sizeof(D3D12_VERTEX_BUFFER_VIEW) * resource_count);

		D3D_PRIMITIVE_TOPOLOGY type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		for (uint32_t res_idx = 0; res_idx < resource_count; ++res_idx)
		{
			if (vertex_resources[res_idx]->GetType() != RenderResourceType::VertexBuffer)
			{
				RB_LOG_ERROR(LOGTAG_GRAPHICS, "Passed in resource %d was not a vertex buffer", res_idx);
				return;
			}

			VertexBufferD3D12* vbo = (VertexBufferD3D12*)vertex_resources[res_idx];

			views[res_idx] = vbo->GetView();

			if (type != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
			{
				RB_ASSERT_FATAL(LOGTAG_GRAPHICS, type == ConvertToD3D12Topology(vbo->GetTopologyType()), "The topology types of the passed in vertex buffers do not match");
			}
			else
			{
				type = ConvertToD3D12Topology(vbo->GetTopologyType());
			}
		}

		m_CommandList->IASetVertexBuffers(start_slot, resource_count, views);
		m_CommandList->IASetPrimitiveTopology(type);

		VertexBufferD3D12* base_vbo = ((VertexBufferD3D12*)vertex_resources[0]);

		m_RenderState.vertexCountPerInstance = base_vbo->GetVertexElementCount();

		D3D12_PRIMITIVE_TOPOLOGY_TYPE current_type = m_RenderState.vertexBufferType;

		switch (base_vbo->GetTopologyType())
		{
		case TopologyType::TriangleList: m_RenderState.vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;

		default:
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Topology type not yet implemented");
			m_RenderState.vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			break;
		}

		if (current_type != m_RenderState.vertexBufferType)
		{
			m_RenderState.psoDirty = true;
		}
	}

	void RenderInterfaceD3D12::CopyResource(RenderResource* src, RenderResource* dest)
	{
		if (src->GetType() != dest->GetType())
		{
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Can not copy resource as the typed do not match");
			return;
		}

		GpuResource* src_res = (GpuResource*)src->GetNativeResource();
		GpuResource* dest_res = (GpuResource*)dest->GetNativeResource();

		switch (src->GetPrimitiveType())
		{
		case RenderResourceType::Buffer:
		{
			InternalCopyBuffer(src_res, dest_res);
		}
		break;

		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
	}

	void RenderInterfaceD3D12::UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size)
	{
		switch (resource->GetPrimitiveType())
		{
		case RenderResourceType::Buffer:
		{
			GpuResource* upload_res = new GpuResource();
			g_ResourceManager->ScheduleCreateUploadResource(upload_res, "Upload resource", data_size);
			
			char* mapped_mem;
			RB_ASSERT_FATAL_D3D(upload_res->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mapped_mem)), "Could not map the upload resource");

			memcpy(mapped_mem, data, data_size);

			// Unmapping is unnecessary
			//upload_res->Unmap(0, nullptr);

			InternalCopyBuffer(upload_res, (GpuResource*)resource->GetNativeResource());

			delete upload_res;
		}
		break;

		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
	}

	void RenderInterfaceD3D12::DrawInternal()
	{
		if (m_RenderState.psoDirty || m_RenderState.rootSignatureDirty)
		{
			SetPipelineState();
		}

		BindResources();

		FlushResourceBarriers();

		if (m_RenderState.indexCountPerInstance > 0)
		{
			m_CommandList->DrawIndexedInstanced(m_RenderState.indexCountPerInstance, 1, 0, 0, 0);
		}
		else
		{
			m_CommandList->DrawInstanced(m_RenderState.vertexCountPerInstance, 1, 0, 0);
		}
	}

	void RenderInterfaceD3D12::DispatchInternal()
	{
	}

	void RenderInterfaceD3D12::BindResources()
	{
		for (int i = 0; i < _countof(m_RenderState.cbvAddresses); ++i)
		{
			if (m_RenderState.cbvAddresses[i] > 0)
			{
				m_CommandList->SetGraphicsRootConstantBufferView(CBV_ROOT_PARAMETER_INDEX_OFFSET + i, m_RenderState.cbvAddresses[i]);
			}
		}
	}

	void RenderInterfaceD3D12::SetPipelineState()
	{
		#define CHECK_SET(check, message) if (!(check)) { RB_LOG_ERROR(LOGTAG_GRAPHICS, message); return; }

		CHECK_SET(m_RenderState.vertexBufferType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,"Cannot draw, vertex buffer was not set")
		CHECK_SET(m_RenderState.numRenderTargets > 0,										"Cannot draw, vertex buffer was not set")
		CHECK_SET(m_RenderState.scissorSet,													"Cannot draw, scissor was not set")
		CHECK_SET(m_RenderState.viewportSet,												"Cannot draw, viewport was not set")
		CHECK_SET(m_RenderState.vsShader >= 0 && m_RenderState.psShader >= 0,				"Cannot draw, vertex or pixel shader was not set yet")
		CHECK_SET(m_RenderState.blendingSet,												"Cannot draw, blend mode was not set")
		CHECK_SET(m_RenderState.rasterizerSet,												"Cannot draw, rasterizer was not set")
		CHECK_SET(m_RenderState.depthStencilSet,											"Cannot draw, depth stencil was not set")
			
		#undef CHECK_SET

		if (m_RenderState.rootSignatureDirty)
		{
			m_RenderState.rootSignature = g_PipelineManager->GetRootSignature(m_RenderState.vsShader, m_RenderState.psShader);

			m_RenderState.rootSignatureDirty = false;
		}
		
		List<D3D12_INPUT_ELEMENT_DESC> input_elements = g_PipelineManager->GetInputElementDesc(m_RenderState.vsShader);

		ShaderBlob* vs_blob = g_ShaderSystem->GetShaderBlob(m_RenderState.vsShader);
		ShaderBlob* ps_blob = g_ShaderSystem->GetShaderBlob(m_RenderState.psShader);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
		pso_desc.pRootSignature			= m_RenderState.rootSignature.Get();
		pso_desc.VS						= { vs_blob->shaderBlob, vs_blob->shaderBlobSize };
		pso_desc.PS						= { ps_blob->shaderBlob, ps_blob->shaderBlobSize };
		//pso_desc.DS					= ;
		//pso_desc.HS					= ;
		//pso_desc.GS					= ;
		//pso_desc.StreamOutput			= ;
		pso_desc.BlendState				= m_RenderState.blendDesc;
		pso_desc.SampleMask				= UINT_MAX;
		pso_desc.RasterizerState		= m_RenderState.rasterizerDesc;
		pso_desc.DepthStencilState		= m_RenderState.depthStencilDesc;
		pso_desc.InputLayout			= { input_elements.data(), (UINT) input_elements.size() };
		//pso_desc.IBStripCutValue		= ;
		pso_desc.PrimitiveTopologyType	= m_RenderState.vertexBufferType;
		pso_desc.NumRenderTargets		= m_RenderState.numRenderTargets;
		/*pso_desc.RTVFormats */		  memcpy(pso_desc.RTVFormats, m_RenderState.rtvFormats, sizeof(DXGI_FORMAT) * 8);
		pso_desc.DSVFormat				= m_RenderState.dsvFormat;
		pso_desc.SampleDesc				= { 1, 0 };
		pso_desc.NodeMask				= 0;
		//pso_desc.CachedPSO			= NULL;
		pso_desc.Flags					= D3D12_PIPELINE_STATE_FLAG_NONE;

		GPtr<ID3D12PipelineState> pso = g_PipelineManager->GetGraphicsPipeline(pso_desc, m_RenderState.vsShader, m_RenderState.psShader);

		m_CommandList->SetPipelineState(pso.Get());
		// Call this before binding any resources to the pipeline (CBV, UAV, SRV, or Samplers)!
		m_CommandList->SetGraphicsRootSignature(m_RenderState.rootSignature.Get());

		m_RenderState.psoDirty = false;
	}

	void RenderInterfaceD3D12::InternalCopyBuffer(GpuResource* src, GpuResource* dest)
	{
		if (!m_CopyOperationsOnly)
		{
			g_ResourceStateManager->TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
			g_ResourceStateManager->TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
			FlushResourceBarriers();
		}
		else
		{
			// We do not need to transition resources to the copy state when using a dedicated copy commandlist.
			// The resources however MUST be in the COMMON state, so check for that here.
			RB_ASSERT_FATAL(LOGTAG_GRAPHICS, src->IsInState(D3D12_RESOURCE_STATE_COMMON), "Source resource was not in the common state before copying");
			RB_ASSERT_FATAL(LOGTAG_GRAPHICS, dest->IsInState(D3D12_RESOURCE_STATE_COMMON), "Destination resource was not in the common state before copying");
		}

		m_CommandList->CopyResource(dest->GetResource().Get(), src->GetResource().Get());
	}
}