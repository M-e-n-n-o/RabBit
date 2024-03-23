#pragma once

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "graphics/d3d12/DeviceQueue.h"
#include "graphics/RenderInterface.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
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
		ResourceManager();
		~ResourceManager();

		void StartFrame();
		void EndFrame();

		void MarkForDelete(GpuResource* resource);
		void OnCommandListExecute(DeviceQueue* queue, uint64_t fence_value);

		bool CreateUploadResource(GpuResource* out_resource, const wchar_t* name, uint64_t size);
		bool CreateVertexResource(GpuResource* out_resource, const wchar_t* name, uint64_t size);

	private:

		// -----------------------------------------------------------------------------
		//								RAW RESOUCE CREATION

		GPtr<ID3D12Resource> CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATES start_state = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* clear_value = nullptr);

	private:
		struct FencePair
		{
			DeviceQueue* queue;
			uint64_t fenceValue;
		};

		UnorderedMap<FencePair, List<GPtr<ID3D12Object>>> m_ObjsWaitingToFinishFlight;
		List<GPtr<ID3D12Object>> m_ObjsScheduledToReleaseAfterExecute;
	};

	extern ResourceManager* g_ResourceManager;
}