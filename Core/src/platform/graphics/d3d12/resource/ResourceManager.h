#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "utils/Threading.h"
#include "platform/graphics/d3d12/RendererD3D12.h"

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
        void FlushBookkeeping();

        void MarkForDelete(GpuResource* resource);

        struct BufferDesc
        {
            uint64_t                size;
            D3D12_HEAP_TYPE         heapType;
            D3D12_RESOURCE_FLAGS    flags;
        };

        struct Texture2DDesc
        {
            DXGI_FORMAT             format;
            uint64_t                width;
            uint64_t                height;
            uint16_t                arraySize;
            uint16_t                mipLevels;
            D3D12_RESOURCE_FLAGS    flags;
        };

        struct Texture3DDesc
        {
            DXGI_FORMAT             format;
            uint64_t                width;
            uint64_t                height;
            uint64_t                depth;
            uint16_t                mipLevels;
            D3D12_RESOURCE_FLAGS    flags;
        };

        void ScheduleCreateBufferResource(GpuResource* resource, const char* name, const BufferDesc& desc);
        void ScheduleCreateTexture2DResource(GpuResource* resource, const char* name, const Texture2DDesc& desc);
        void ScheduleCreateTexture3DResource(GpuResource* resource, const char* name, const Texture3DDesc& desc);

        bool WaitUntilResourceValid(const GpuResource* resource);

    private:

        // -----------------------------------------------------------------------------
        //								RAW RESOUCE CREATION

        ID3D12Resource* CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT,
            D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATES start_state = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* optimized_clear_value = nullptr);

        enum class ResourceType
        {
            Buffer,
            Texture2D,
            Texture3D
        };

        struct ResourceCreationDesc : public JobData
        {
            ResourceType    type;
            GpuResource*    resource;
            const wchar_t*  name;

            union
            {
                BufferDesc      buffer;
                Texture2DDesc   tex2D;
                Texture3DDesc   tex3D;
            };

            ~ResourceCreationDesc()
            {
                delete[] name;
            }
        };

        struct ResourceDeletionDesc : public JobData
        {
            ID3D12Object** objects;
            uint32_t       count;

            ~ResourceDeletionDesc()
            {
                SAFE_FREE(objects);
            }
        };

        struct Scheduled
        {
            GpuResource* resource;
            JobID        jobID;
        };

        WorkerThread*       m_CreationThread;
        JobTypeID           m_CreationJob;
        JobTypeID           m_DeletionJob;
        List<Scheduled>     m_ScheduledCreations;
        List<ID3D12Object*> m_ScheduledDeletions[TRANSIENT_CYCLES];
        uint32_t            m_CurrentDeletionList;
        Mutex               m_Mutex;

        extern friend void CreationJob(JobData* data);
        extern friend void ReleaseJob(JobData* data);
    };

    extern ResourceManager* g_ResourceManager;
}
#endif