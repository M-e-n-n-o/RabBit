#pragma once

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "graphics/d3d12/DeviceQueue.h"
#include "graphics/RenderInterface.h"
#include "utils/Threading.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    /*
    * TODO:
    * - Track the resource sizes
    * - Make is possible to easily create and alias (multiple) resources from 1 heap (all rendertargets in 1 big heap?)
    */

    // Global resource manager
    class ResourceManager
    {
    public:
        ResourceManager();
        ~ResourceManager();

        void UpdateBookkeeping();

        void MarkUsed(GpuResource* resource, DeviceQueue* queue);

        void MarkForDelete(GpuResource* resource);
        void OnCommandListExecute(DeviceQueue* queue, uint64_t fence_value);

        struct BufferDesc
        {
            uint64_t				size;
        };

        struct Texture2DDesc
        {
            DXGI_FORMAT				format;
            uint64_t				width;
            uint64_t				height;
            uint16_t				arraySize;
            uint16_t				mipLevels;
            D3D12_RESOURCE_FLAGS	flags;
        };

        void ScheduleCreateUploadResource(GpuResource* resource, const char* name, const BufferDesc& desc);
        void ScheduleCreateVertexResource(GpuResource* resource, const char* name, const BufferDesc& desc);
        void ScheduleCreateIndexResource(GpuResource* resource, const char* name, const BufferDesc& desc);
        void ScheduleCreateTexture2DResource(GpuResource* resource, const char* name, const Texture2DDesc& desc);

        bool WaitUntilResourceValid(GpuResource* resource);

    private:

        // -----------------------------------------------------------------------------
        //								RAW RESOUCE CREATION

        GPtr<ID3D12Resource> CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT,
            D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATES start_state = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* optimized_clear_value = nullptr);

        struct FencePair
        {
            DeviceQueue* queue;
            uint64_t	 fenceValue;
        };

        UnorderedMap<DeviceQueue*, List<GPtr<ID3D12Object>>> m_ScheduledUsages;
        UnorderedMap<FencePair*,   List<GPtr<ID3D12Object>>> m_InFlight;

        enum class ResourceType
        {
            Upload,
            Vertex,
            Index,
            Texture2D
        };

        struct ResourceCreationDesc : public JobData
        {
            ResourceType	type;
            GpuResource*    resource;
            const wchar_t*  name;

            union
            {
                BufferDesc		buffer;
                Texture2DDesc	tex2D;
            };

            ~ResourceCreationDesc()
            {
                delete[] name;
            }
        };

        struct Scheduled
        {
            GpuResource* resource;
            JobID		 jobID;
        };

        WorkerThread*       m_CreationThread;
        JobTypeID			m_CreationJob;
        List<Scheduled>		m_ScheduledCreations;

        CRITICAL_SECTION	m_CS;

        extern friend void CreationJob(JobData* data);
    };

    extern ResourceManager* g_ResourceManager;
}