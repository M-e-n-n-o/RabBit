#pragma once

#include "RabBitCommon.h"
#include "Resource.h"

#include <d3d12.h>

namespace RB::Graphics::Native
{
	/*
	* TODO: 
	* - Track the resource sizes
	* - Make is possible to easily create and alias (multiple) resources from 1 heap (all rendertargets in 1 big heap)
	* - Make sure, there is a way to detect which resources should be deleted, and delete them (also remove from ResourceStateManager) 
		(keep all vertex data in memory, stream textures and shader inputs)
	*/

	// Global resource manager
	class ResourceManager
	{
	public:
		ResourceManager(uint32_t total_back_buffers);	

		void StartFrame(uint32_t back_buffer_index);
		void EndFrame();

		void MarkAsUsed(Resource* resource);

		void MarkForDelete(Resource* resource);

		void CreateTexture2D();

		// -----------------------------------------------------------------------------
		//								RAW RESOUCE CREATION

		GPtr<ID3D12Resource> CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATES start_state = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* clear_value = nullptr);
	};

	extern ResourceManager* g_ResourceManager;


	class SimpleDescriptorHeap
	{
	public:
		SimpleDescriptorHeap(const wchar_t* name, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_number_descriptors, bool shader_visible);
		SimpleDescriptorHeap(const SimpleDescriptorHeap& other) = delete;
		~SimpleDescriptorHeap();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t offset);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t offset);

		bool IsShaderVisible() const { return m_IsShaderVisible; }

		GPtr<ID3D12DescriptorHeap> GetHeap() const { return m_Heap; }

	private:
		GPtr<ID3D12DescriptorHeap>	m_Heap;

		SIZE_T						m_CpuStart;
		SIZE_T						m_GpuStart;

		const wchar_t*				m_Name;
		bool						m_IsShaderVisible;
		uint32_t					m_MaxDescriptors;
		uint32_t					m_IncrementSize;
	};
}