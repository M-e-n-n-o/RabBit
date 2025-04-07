#include "ResourceManager.h"
#include "RabBitCommon.h"
#include "../GraphicsDevice.h"
#include "ResourceManager.h"
#include "ResourceStateManager.h"

namespace RB::Graphics::D3D12
{
    void CreationJob(JobData* data);

    ResourceManager* g_ResourceManager = nullptr;

    ResourceManager::ResourceManager()
    {
        InitializeCriticalSection(&m_CS);

        m_CreationThread = new WorkerThread(L"Resource Creation Thread", ThreadPriority::Medium);

        m_CreationJob = m_CreationThread->AddJobType(&CreationJob, false);
    }

    ResourceManager::~ResourceManager()
    {
        // Waits until the thread is done with all tasks
        delete m_CreationThread;

        // Wait until all objects can be release
        for (auto itr = m_InFlight.begin(); itr != m_InFlight.end();)
        {
            itr->first->queue->CpuWaitForFenceValue(itr->first->fenceValue);
            itr = m_InFlight.erase(itr);
        }

        DeleteCriticalSection(&m_CS);
    }

    void ResourceManager::UpdateBookkeeping()
    {
        EnterCriticalSection(&m_CS);

        // TODO Future improvement would be to free the resources on the ResourceCreation thread as this can also be pretty expensive

        // Check which resources have finished executing on the GPU
        for (auto itr = m_InFlight.begin(); itr != m_InFlight.end();)
        {
            if (itr->first->queue->IsFenceReached(itr->first->fenceValue))
            {
                delete itr->first;
                itr = m_InFlight.erase(itr);
            }
            else
            {
                ++itr;
            }
        }

        // Check which scheduled creations are completed
        for (auto itr = m_ScheduledCreations.begin(); itr < m_ScheduledCreations.end();)
        {
            if (m_CreationThread->IsFinished(itr->jobID))
            {
                itr = m_ScheduledCreations.erase(itr);
            }
            else
            {
                ++itr;
            }
        }

        LeaveCriticalSection(&m_CS);
    }

    void ResourceManager::MarkUsed(GpuResource* resource, DeviceQueue* queue)
    {
        if (!resource->IsValid())
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Resource cannot be set as used, not valid");
            return;
        }

        EnterCriticalSection(&m_CS);

        auto itr = m_ScheduledUsages.find(queue);

        if (itr == m_ScheduledUsages.end())
        {
            List<GPtr<ID3D12Object>> list;
            list.push_back(resource->GetResource());

            m_ScheduledUsages.emplace(queue, list);
        }
        else
        {
            // Only add it if its not already in the list
            if (std::find(itr->second.begin(), itr->second.end(), resource->GetResource()) == itr->second.end())
            {
                itr->second.push_back(resource->GetResource());
            }
        }

        LeaveCriticalSection(&m_CS);
    }

    void ResourceManager::MarkForDelete(GpuResource* resource)
    {
        // Don't need to do anything to delete at the right time. This because when the resource is currently in flight or still 
        // has to be used by a command list, there is a reference being kept to it by the m_ScheduledUsages or the m_InFlight member.
    }

    void ResourceManager::OnCommandListExecute(DeviceQueue* queue, uint64_t fence_value)
    {
        EnterCriticalSection(&m_CS);

        auto scheduled_use_itr = m_ScheduledUsages.find(queue);
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, scheduled_use_itr != m_ScheduledUsages.end(), "DeviceQueue was not yet registered for the scheduled usages queue");

        FencePair* fence_pair = new FencePair();
        fence_pair->queue       = queue;
        fence_pair->fenceValue  = fence_value;

        // Set all resources that were marked for use to this queue in flight
        m_InFlight.emplace(fence_pair, scheduled_use_itr->second);

        // Clear the scheduled usages for this queue
        scheduled_use_itr->second.clear();

        LeaveCriticalSection(&m_CS);
    }

    bool ResourceManager::WaitUntilResourceValid(GpuResource* resource)
    {
        if (m_CreationThread->IsCurrentThread())
        {
            // Its possible the creation thread might get in here because of the creation callback of the GpuResource
            return true;
        }

        EnterCriticalSection(&m_CS);

        // Find the scheduled creation
        auto itr = std::find_if(m_ScheduledCreations.begin(), m_ScheduledCreations.end(), [resource](const Scheduled& scheduled) -> bool {
            return resource == scheduled.resource;
            });

        if (itr == m_ScheduledCreations.end())
        {
            LeaveCriticalSection(&m_CS);
            return false;
        }

        // Tell the thread to prioritize this resource creation as we are waiting for it
        m_CreationThread->PrioritizeJob(itr->jobID);

        JobID id = itr->jobID;
        m_ScheduledCreations.erase(itr);

        LeaveCriticalSection(&m_CS);

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

        EnterCriticalSection(&m_CS);

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);
        m_ScheduledCreations.push_back({ resource, id });

        LeaveCriticalSection(&m_CS);
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

        EnterCriticalSection(&m_CS);

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);
        m_ScheduledCreations.push_back({ resource, id });

        LeaveCriticalSection(&m_CS);
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

        EnterCriticalSection(&m_CS);

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);
        m_ScheduledCreations.push_back({ resource, id });

        LeaveCriticalSection(&m_CS);
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

        EnterCriticalSection(&m_CS);

        JobID id = m_CreationThread->ScheduleJob(m_CreationJob, desc);
        m_ScheduledCreations.push_back({ resource, id });

        LeaveCriticalSection(&m_CS);
    }

    GPtr<ID3D12Resource> ResourceManager::CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
        D3D12_HEAP_FLAGS heap_flags, D3D12_RESOURCE_STATES start_state, const D3D12_CLEAR_VALUE* optimized_clear_value)
    {
        GPtr<ID3D12Resource> resource = nullptr;

        RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(heap_type),
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
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;

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
}