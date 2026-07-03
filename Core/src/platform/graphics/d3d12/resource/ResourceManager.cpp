#if RB_GRAPHICS_API_D3D12

#include "ResourceManager.h"
#include "RabBitCommon.h"
#include "../GraphicsDevice.h"
#include "ResourceManager.h"
#include "ResourceStateManager.h"

#include <d3dx12/d3dx12.h>

namespace RB::Graphics::D3D12
{
    void CreationJob(JobData* data);
    void ReleaseJob(JobData* data);

    ResourceManager* g_ResourceManager = nullptr;

    ResourceManager::ResourceManager()
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

        std::erase_if(m_ScheduledCreations, [this](const auto& creation)
            {
                return m_CreationThread->IsFinished(creation.jobID);
            });
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

    bool ResourceManager::WaitUntilResourceValid(const GpuResource* resource)
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
            auto itr = std::find_if(m_ScheduledCreations.begin(), m_ScheduledCreations.end(), [resource](const Scheduled& scheduled) -> bool {
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

    void ResourceManager::ScheduleCreateUploadResource(GpuResource* resource, const char* name, const BufferDesc& buffer_desc)
    {
        // Is deleted in destructor of ResourceCreationDesc
        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Upload;
        desc->resource  = resource;
        desc->name      = wname;
        desc->buffer    = buffer_desc;

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledCreations.push_back({ resource, id });
    }

    void ResourceManager::ScheduleCreateReadbackResource(GpuResource* resource, const char* name, const BufferDesc& buffer_desc)
    {
        // Is deleted in destructor of ResourceCreationDesc
        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Readback;
        desc->resource  = resource;
        desc->name      = wname;
        desc->buffer    = buffer_desc;

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledCreations.push_back({ resource, id });
    }

    void ResourceManager::ScheduleCreateVertexResource(GpuResource* resource, const char* name, const BufferDesc& buffer_desc)
    {
        // Is deleted in destructor of ResourceCreationDesc
        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Vertex;
        desc->resource  = resource;
        desc->name      = wname;
        desc->buffer    = buffer_desc;

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledCreations.push_back({ resource, id });
    }

    void ResourceManager::ScheduleCreateIndexResource(GpuResource* resource, const char* name, const BufferDesc& buffer_desc)
    {
        // Is deleted in destructor of ResourceCreationDesc
        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Index;
        desc->resource  = resource;
        desc->name      = wname;
        desc->buffer    = buffer_desc;

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);

        RB_MUTEX_AUTO_LOCK(m_Mutex);
        m_ScheduledCreations.push_back({ resource, id });
    }

    void ResourceManager::ScheduleCreateTexture2DResource(GpuResource* resource, const char* name, const Texture2DDesc& tex_desc)
    {
        // Is deleted in destructor of ResourceCreationDesc
        wchar_t* wname = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wname);

        ResourceCreationDesc* desc = new ResourceCreationDesc();
        desc->type      = ResourceType::Texture2D;
        desc->resource  = resource;
        desc->name      = wname;
        desc->tex2D     = tex_desc;

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

        switch (creation_desc->type)
        {
        case ResourceManager::ResourceType::Upload:
        {
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

            creation_desc->resource->SetResource(
                g_ResourceManager->CreateCommittedResource(
                    creation_desc->name,
                    CD3DX12_RESOURCE_DESC::Buffer(creation_desc->buffer.size),
                    D3D12_HEAP_TYPE_UPLOAD,
                    D3D12_HEAP_FLAG_NONE,
                    state),
                state);
        }
        break;

        case ResourceManager::ResourceType::Readback:
        {
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;

            creation_desc->resource->SetResource(
                g_ResourceManager->CreateCommittedResource(
                    creation_desc->name,
                    CD3DX12_RESOURCE_DESC::Buffer(creation_desc->buffer.size),
                    D3D12_HEAP_TYPE_READBACK,
                    D3D12_HEAP_FLAG_NONE,
                    state),
                state);
        }
        break;

        case ResourceManager::ResourceType::Vertex:
        case ResourceManager::ResourceType::Index:
        {
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON; //D3D12_RESOURCE_STATE_COPY_DEST // Buffers are always created in the common state

            creation_desc->resource->SetResource(
                g_ResourceManager->CreateCommittedResource(
                    creation_desc->name,
                    CD3DX12_RESOURCE_DESC::Buffer(creation_desc->buffer.size),
                    D3D12_HEAP_TYPE_DEFAULT,
                    D3D12_HEAP_FLAG_NONE, //D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, // This flag is not allowed for commited resources as they are set automatically
                    state),
                state);
        }
        break;

        case ResourceManager::ResourceType::Texture2D:
        {
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

            // TODO Fill in the optimized clear value for RenderTargets and DepthStencil textures

            creation_desc->resource->SetResource(
                g_ResourceManager->CreateCommittedResource(
                    creation_desc->name,
                    CD3DX12_RESOURCE_DESC::Tex2D(creation_desc->tex2D.format, creation_desc->tex2D.width, creation_desc->tex2D.height,
                        creation_desc->tex2D.arraySize, creation_desc->tex2D.mipLevels, 1, 0, creation_desc->tex2D.flags, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0),
                    D3D12_HEAP_TYPE_DEFAULT,
                    D3D12_HEAP_FLAG_NONE,
                    state),
                state);
        }
        break;

        default:
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Not yet implemented resource creation of this type");
        }
        break;
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