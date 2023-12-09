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

	class ResourceUploader;

	// Increase if planning to upload more data to the GPU than the following bytes at a time
	const uint32_t kUploadPageSize = 4096;


	// NOTE: THE WAY THIS IS SET UP, WHEN ESSENTIALLY UPLOADING DATA EVERY FRAME, THAT DATA WILL BE DOUBLE BUFFERED, DO WE REALLY WANT AND NEED THIS????
	// It is preferably to not use this upload allocation directly for a draw/compute call, but to copy the data first 
	// into a more stable resource, as this resource will reset after every 'total_back_buffers' frames (+ is an upload resource, so less performance).
	// However, if the data should change every frame, using this resource directly in a draw/compute call can be better.
	struct UploadAllocation
	{
		GPtr<ID3D12Resource>		entireUploadResource;
		D3D12_GPU_VIRTUAL_ADDRESS	offsetedGpuAddress;
		uint64_t					offsetInResource;
		uint64_t					maxWriteSize;
		void*						cpuWriteLocation;
	};

	// Global resource manager
	class ResourceManager
	{
	public:
		ResourceManager(uint32_t total_back_buffers);	
		~ResourceManager();

		void StartFrame(uint32_t back_buffer_index);
		void EndFrame();

		UploadAllocation UploadData(void* data, uint32_t size, uint32_t alignment);

		void MarkAsUsed(Resource* resource);

		void MarkForDelete(Resource* resource);

		void CreateTexture2D();

		// -----------------------------------------------------------------------------
		//								RAW RESOUCE CREATION

		GPtr<ID3D12Resource> CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATES start_state = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* clear_value = nullptr);

	private:
		ResourceUploader**	m_ResourceUploaders;
		ResourceUploader*	m_ActiveResourceUploader;
	};

	extern ResourceManager* g_ResourceManager;
}