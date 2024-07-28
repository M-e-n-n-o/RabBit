#include "RabBitCommon.h"
#include "DescriptorManager.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	DescriptorManager* g_DescriptorManager = nullptr;

	DescriptorManager::DescriptorManager()
	{
		m_BindlessSrvUavHeap = new DescriptorHeap(L"Bindless SRV/UAV heap", true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, BINDLESS_DESCRIPTOR_HEAP_SIZE);
	}

	DescriptorManager::~DescriptorManager()
	{
		SAFE_DELETE(m_BindlessSrvUavHeap);
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

	void DescriptorManager::InvalidateDescriptor(DescriptorHandle& handle)
	{
		m_BindlessSrvUavHeap->InvalidateDescriptor(handle);
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