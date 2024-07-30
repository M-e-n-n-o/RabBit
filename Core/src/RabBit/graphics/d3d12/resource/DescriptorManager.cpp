#include "RabBitCommon.h"
#include "DescriptorManager.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	DescriptorManager* g_DescriptorManager = nullptr;

	DescriptorManager::DescriptorManager()
	{
		m_BindlessSrvUavHeap = new DescriptorHeap(L"Bindless SRV/UAV heap", true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, BINDLESS_DESCRIPTOR_HEAP_SIZE);
		m_RenderTargetHeap	 = new DescriptorHeap(L"RenderTarget heap", false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RENDERTARGET_DESCRIPTOR_HEAP_SIZE);
		m_DepthStencilHeap	 = new DescriptorHeap(L"Depth Stencil heap", false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DEPTHSTENCIL_DESCRIPTOR_HEAP_SIZE);
	}

	DescriptorManager::~DescriptorManager()
	{
		SAFE_DELETE(m_BindlessSrvUavHeap);
		SAFE_DELETE(m_RenderTargetHeap);
		SAFE_DELETE(m_DepthStencilHeap);
	}

	DescriptorHandle DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE staging_handle = m_BindlessSrvUavHeap->GetStagingDestinationDescriptor();

		g_GraphicsDevice->Get()->CreateShaderResourceView(res, &desc, staging_handle);

		return m_BindlessSrvUavHeap->InsertStagedDescriptor(BINDLESS_TEX2D_START_OFFSET, BINDLESS_TEX2D_DESCRIPTORS);
	}

	DescriptorHandle DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE staging_handle = m_BindlessSrvUavHeap->GetStagingDestinationDescriptor();

		g_GraphicsDevice->Get()->CreateUnorderedAccessView(res, nullptr, &desc, staging_handle);

		return m_BindlessSrvUavHeap->InsertStagedDescriptor(BINDLESS_RWTEX2D_START_OFFSET, BINDLESS_RWTEX2D_DESCRIPTORS);
	}

	DescriptorHandle DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
	{
		DescriptorHandle handle;
		D3D12_CPU_DESCRIPTOR_HANDLE dest_handle = m_RenderTargetHeap->GetDestinationDescriptor(handle, 0, RENDERTARGET_DESCRIPTOR_HEAP_SIZE);

		g_GraphicsDevice->Get()->CreateRenderTargetView(res, &desc, dest_handle);

		return handle;
	}

	DescriptorHandle DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
	{
		DescriptorHandle handle;
		D3D12_CPU_DESCRIPTOR_HANDLE dest_handle = m_DepthStencilHeap->GetDestinationDescriptor(handle, 0, DEPTHSTENCIL_DESCRIPTOR_HEAP_SIZE);

		g_GraphicsDevice->Get()->CreateDepthStencilView(res, &desc, dest_handle);

		return handle;
	}

	void DescriptorManager::InvalidateDescriptor(DescriptorHandle& handle, const DescriptorHandleType& type)
	{
		switch (type)
		{
		case DescriptorHandleType::SRV:
		case DescriptorHandleType::UAV:
		{
			m_BindlessSrvUavHeap->InvalidateDescriptor(handle);
			break;
		}
		case DescriptorHandleType::RTV:
		{
			m_RenderTargetHeap->InvalidateDescriptor(handle);
			break;
		}
		case DescriptorHandleType::DSV:
		{
			m_DepthStencilHeap->InvalidateDescriptor(handle);
			break;
		}
		case DescriptorHandleType::CBV:
		default:
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Descriptor handle type not valid");
			break;
		}
	}

	Array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorManager::GetHeaps(uint32_t& num_heaps)
	{
		Array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> arr;

		arr[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = m_BindlessSrvUavHeap->GetHeap().Get();
		arr[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]		= nullptr;
		arr[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]			= nullptr;
		arr[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]			= nullptr;

		num_heaps = 1;

		return arr;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetTex2DStart() const
	{
		return m_BindlessSrvUavHeap->GetGpuStart(BINDLESS_TEX2D_START_OFFSET);
	}
}