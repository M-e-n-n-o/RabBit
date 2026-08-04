#if RB_GRAPHICS_API_D3D12

#include "ResourceManager.h"
#include "RabBitCommon.h"
#include "ResourceStateManager.h"

#include "../GraphicsDevice.h"
#include "../UtilsD3D12.h"

#include "graphics/RenderInterface.h"

#include <d3dx12/d3dx12.h>

namespace RB::Graphics::D3D12
{
    void CreationJob(JobData* data);
    void ReleaseJob(JobData* data);

    ResourceManager* g_ResourceManager = nullptr;

    ResourceManager::ResourceManager(RenderInterface* render_interface)
        : m_RenderInterface(render_interface)
    {
        m_CreationThread = new WorkerThread("Resource Manager", ThreadPriority::Medium);

        m_CurrentDeletionList = 0;

        m_CreationJob = m_CreationThread->AddJobType(&CreationJob, false);
        m_DeletionJob = m_CreationThread->AddJobType(&ReleaseJob, false);
    }

    ResourceManager::~ResourceManager()
    {
        FlushBookkeeping();

        // Waits until the thread is done with all tasks
        delete m_CreationThread;
    }

    void ResourceManager::UpdateBookkeeping()
    {
        RB_MUTEX_AUTO_LOCK(m_Mutex);

        std::erase_if(m_ScheduledCreations, [this](const auto& creation)
            {
                return m_CreationThread->IsFinished(creation.jobID);
            });

        m_CurrentDeletionList = (m_CurrentDeletionList + 1) % TRANSIENT_CYCLES;

        auto& list = m_ScheduledDeletions[m_CurrentDeletionList];

        if (list.empty())
        {
            return;
        }

        uint32_t size = sizeof(ID3D12Object*) * list.size();

        ResourceDeletionDesc* desc = new ResourceDeletionDesc();
        desc->objects = (ID3D12Object**)ALLOC_HEAP(size);
        desc->count   = list.size();

        memcpy(desc->objects, list.data(), size);

        m_CreationThread->ScheduleJob(m_DeletionJob, desc);

        list.clear();
    }

    void ResourceManager::FlushBookkeeping()
    {
        for (int i = 0; i < TRANSIENT_CYCLES; ++i)
        {
            UpdateBookkeeping();
        }
    }

    void ResourceManager::MarkForDelete(GpuResource* resource)
    {
        // Get the resource before aquiring the lock as its possible that we have to wait until the resource is created
        ID3D12Resource* res = resource->GetResource();

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledDeletions[m_CurrentDeletionList].push_back(res);
    }

    bool ResourceManager::WaitUntilResourceCreated(const GpuResource* resource)
    {
        if (m_CreationThread->IsCurrentThread())
        {
            // Its possible the creation thread might get in here because of the creation callback of the GpuResource
            return true;
        }

        JobID id = -1;
        {
            RB_MUTEX_AUTO_LOCK(m_Mutex);

            // Find the scheduled creation
            auto itr = std::find_if(m_ScheduledCreations.begin(), m_ScheduledCreations.end(), [resource](const Scheduled& scheduled) -> bool 
                {
                    return resource == scheduled.resource;
                });

            if (itr == m_ScheduledCreations.end())
            {
                return false;
            }

            id = itr->jobID;

            // Tell the thread to prioritize this resource creation as we are waiting for it
            m_CreationThread->PrioritizeJob(id);
        }

        // Sync until the job is completed
        m_CreationThread->Sync(id);

        return true;
    }

    void ResourceManager::ScheduleCreateBufferResource(GpuResource* resource, const char* name, const BufferDesc& buffer_desc, const void* data, uint64_t data_size)
    {
        // Memory is free'd in the destructor of ResourceCreationDesc

        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Buffer;
        desc->resource  = resource;
        desc->name      = wname;
        desc->buffer    = buffer_desc;
        desc->data      = nullptr;
        desc->dataSize  = data_size;
        desc->ri        = m_RenderInterface;

        if (data && data_size > 0)
        {
            void* data_copy = ALLOC_HEAP(data_size);
            memcpy(data_copy, data, data_size);
            desc->data = data_copy;
        }

        if (m_CreationThread->IsCurrentThread())
        {
            // If the creation thread itself tries to create a resource (an upload buffer) we just instantly create it
            CreationJob(desc);
            desc->OnDestroy(false);
            delete desc;
        }
        else
        {
            JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

            RB_MUTEX_AUTO_LOCK(m_Mutex);
            m_ScheduledCreations.push_back({ resource, id });
        }
    }

    void ResourceManager::ScheduleCreateTexture2DResource(GpuResource* resource, const char* name, const Texture2DDesc& tex_desc, const void* data, uint64_t data_size)
    {
        // Memory is free'd in the destructor of ResourceCreationDesc

        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Texture2D;
        desc->resource  = resource;
        desc->name      = wname;
        desc->tex2D     = tex_desc;
        desc->data      = nullptr;
        desc->dataSize  = data_size;
        desc->ri        = m_RenderInterface;
        
        if (data && data_size > 0)
        {
            void* data_copy = ALLOC_HEAP(data_size);
            memcpy(data_copy, data, data_size);
            desc->data = data_copy;
        }

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledCreations.push_back({ resource, id });
    }

    void ResourceManager::ScheduleCreateTexture3DResource(GpuResource* resource, const char* name, const Texture3DDesc& tex_desc, const void* data, uint64_t data_size)
    {
        // Memory is free'd in the destructor of ResourceCreationDesc

        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Texture3D;
        desc->resource  = resource;
        desc->name      = wname;
        desc->tex3D     = tex_desc;
        desc->data      = nullptr;
        desc->dataSize  = data_size;
        desc->ri        = m_RenderInterface;
        
        if (data && data_size > 0)
        {
            void* data_copy = ALLOC_HEAP(data_size);
            memcpy(data_copy, data, data_size);
            desc->data = data_copy;
        }

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledCreations.push_back({ resource, id });
    }

    ID3D12Resource* ResourceManager::CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
        D3D12_HEAP_FLAGS heap_flags, D3D12_RESOURCE_STATES start_state, const D3D12_CLEAR_VALUE* optimized_clear_value)
    {
        ID3D12Resource* resource = nullptr;

        auto props = CD3DX12_HEAP_PROPERTIES(heap_type);

        RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateCommittedResource(
            &props,
            heap_flags,
            &resource_desc,
            start_state,
            optimized_clear_value,
            IID_PPV_ARGS(&resource)
        ), "Could not create committed resource: %s", name);

        if (resource)
        {
            resource->SetName(name);

            RB_ASSERT_FATAL(LOGTAG_GRAPHICS, g_ResourceStateManager, "The resource state manager does not yet exist");
        }

        return resource;
    }

    void CreationJob(JobData* data)
    {
        ResourceManager::ResourceCreationDesc* creation_desc = (ResourceManager::ResourceCreationDesc*)data;

        ID3D12Resource* resource = nullptr;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        switch (creation_desc->type)
        {
        case ResourceManager::ResourceType::Buffer:
        {
            state = D3D12_RESOURCE_STATE_COMMON;
            if (creation_desc->buffer.heapType == D3D12_HEAP_TYPE_READBACK)
                state = D3D12_RESOURCE_STATE_COPY_DEST;

            resource = g_ResourceManager->CreateCommittedResource(creation_desc->name,
                                                                  CD3DX12_RESOURCE_DESC::Buffer(creation_desc->buffer.size, creation_desc->buffer.flags),
                                                                  creation_desc->buffer.heapType,
                                                                  D3D12_HEAP_FLAG_NONE,
                                                                  state);
        }
        break;

        case ResourceManager::ResourceType::Texture2D:
        {
            state = D3D12_RESOURCE_STATE_COMMON;

            // TODO Fill in the optimized clear value for RenderTargets and DepthStencil textures

            resource = g_ResourceManager->CreateCommittedResource(creation_desc->name,
                                                                  CD3DX12_RESOURCE_DESC::Tex2D(ConvertToDXGIFormat(creation_desc->tex2D.format, false), 
                                                                      creation_desc->tex2D.width, creation_desc->tex2D.height,
                                                                      creation_desc->tex2D.arraySize, creation_desc->tex2D.mipLevels, 1, 0, 
                                                                      creation_desc->tex2D.flags, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0),
                                                                  D3D12_HEAP_TYPE_DEFAULT,
                                                                  D3D12_HEAP_FLAG_NONE,
                                                                  state);
        }
        break;

        case ResourceManager::ResourceType::Texture3D:
        {
            state = D3D12_RESOURCE_STATE_COMMON;

            resource = g_ResourceManager->CreateCommittedResource(creation_desc->name,
                                                                  CD3DX12_RESOURCE_DESC::Tex3D(ConvertToDXGIFormat(creation_desc->tex3D.format, false), 
                                                                      creation_desc->tex3D.width, creation_desc->tex3D.height, creation_desc->tex3D.depth,
                                                                      creation_desc->tex3D.mipLevels, creation_desc->tex3D.flags, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0),
                                                                  D3D12_HEAP_TYPE_DEFAULT,
                                                                  D3D12_HEAP_FLAG_NONE,
                                                                  state);
        }
        break;

        default:
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Not yet implemented resource creation of this type");
        }
        break;
        }

        // Upload data to the resources
        if (creation_desc->data != nullptr && creation_desc->dataSize > 0)
        {
            switch (creation_desc->type)
            {
            case ResourceManager::ResourceType::Buffer: 
                creation_desc->ri->UploadDataToResource(resource, RenderResourceType::Buffer, RenderResourceFormat::Unkown, creation_desc->data, creation_desc->dataSize);
                break;
            case ResourceManager::ResourceType::Texture2D:
                creation_desc->ri->UploadDataToResource(resource, RenderResourceType::Texture, creation_desc->tex2D.format, creation_desc->data, creation_desc->dataSize);
                break;
            case ResourceManager::ResourceType::Texture3D: 
                creation_desc->ri->UploadDataToResource(resource, RenderResourceType::Texture, creation_desc->tex3D.format, creation_desc->data, creation_desc->dataSize);
                break;
            default:
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Not yet implemented resource upload of this type");
                break;
            }

            // Bit bad to have an execute for every upload, but this was easiest and cleanest for now, TODO: Batch uploads together!
            Shared<GpuGuard> guard = creation_desc->ri->ExecuteOnGpu();
            creation_desc->resource->SetResource(resource, state, guard);
        }
        else
        {
            creation_desc->resource->SetResource(resource, state, nullptr);
        }
    }

    void ReleaseJob(JobData* data)
    {
        ResourceManager::ResourceDeletionDesc* deletion_desc = (ResourceManager::ResourceDeletionDesc*)data;

        for (int i = 0; i < deletion_desc->count; ++i)
        {
            SAFE_RELEASE(deletion_desc->objects[i]);
        }
    }
}
#endif