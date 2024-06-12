#include "ResourceManager.h"
#include "RabBitCommon.h"
#include "../GraphicsDevice.h"
#include "ResourceManager.h"
#include "ResourceStateManager.h"

namespace RB::Graphics::D3D12
{
	//// ----------------------------------------------------------------------------
	////								Simple descriptor heap
	//// ----------------------------------------------------------------------------

	//class SimpleDescriptorHeap
	//{
	//public:
	//	SimpleDescriptorHeap(const wchar_t* name, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_number_descriptors, bool shader_visible)
	//		: m_MaxDescriptors(max_number_descriptors)
	//		, m_IsShaderVisible(shader_visible)
	//		, m_Name(name)
	//	{
	//		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	//		desc.Type			= type;
	//		desc.NumDescriptors = max_number_descriptors;
	//		desc.Flags			= shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//		desc.NodeMask		= 0;

	//		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)), "Could not create descriptor heap: %ws", name);

	//		m_Heap->SetName(name);

	//		m_IncrementSize = g_GraphicsDevice->Get()->GetDescriptorHandleIncrementSize(type);

	//		m_CpuStart = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr;
	//		m_GpuStart = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr;
	//	}

	//	SimpleDescriptorHeap(const SimpleDescriptorHeap& other) = delete;
	//	~SimpleDescriptorHeap() = default;

	//	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t offset)
	//	{
	//		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, offset < m_MaxDescriptors && offset >= 0, L"Given offset to retrieve the CPU handle of the descriptor heap did not fall in the range of the max number of descriptors of heap: %ws", m_Name);

	//		return { m_CpuStart + (m_IncrementSize * offset) };
	//	}

	//	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t offset)
	//	{
	//		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, offset < m_MaxDescriptors && offset >= 0, L"Given offset to retrieve the GPU handle of the descriptor heap did not fall in the range of the max number of descriptors of heap: %ws", m_Name);

	//		return { m_GpuStart + (m_IncrementSize * offset) };
	//	}

	//	bool IsShaderVisible() const { return m_IsShaderVisible; }

	//	GPtr<ID3D12DescriptorHeap> GetHeap() const { return m_Heap; }

	//private:
	//	GPtr<ID3D12DescriptorHeap>	m_Heap;

	//	SIZE_T						m_CpuStart;
	//	SIZE_T						m_GpuStart;

	//	const wchar_t*				m_Name;
	//	bool						m_IsShaderVisible;
	//	uint32_t					m_MaxDescriptors;
	//	uint32_t					m_IncrementSize;
	//};

	// ----------------------------------------------------------------------------
	//								ResourceManager
	// ----------------------------------------------------------------------------

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
			itr->first.queue->CpuWaitForFenceValue(itr->first.fenceValue);
			itr = m_InFlight.erase(itr);
		}

		DeleteCriticalSection(&m_CS);
	}

	void ResourceManager::StartFrame()
	{

	}

	void ResourceManager::EndFrame()
	{
		EnterCriticalSection(&m_CS);

		// Check which resources have finished executing on the GPU
		for (auto itr = m_InFlight.begin(); itr != m_InFlight.end();)
		{
			if (itr->first.queue->IsFenceReached(itr->first.fenceValue))
			{
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
			itr->second.push_back(resource->GetResource());
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

		FencePair fence_pair;
		fence_pair.queue	  = queue;
		fence_pair.fenceValue = fence_value;

		// Set all resources that were marked for use to this queue in flight
		m_InFlight.emplace(fence_pair, scheduled_use_itr->second);

		// Clear the scheduled usages for this queue
		scheduled_use_itr->second.clear();

		LeaveCriticalSection(&m_CS);
	}

	bool ResourceManager::WaitUntilResourceValid(GpuResource* resource)
	{
		// Find the scheduled creation
		auto itr = std::find_if(m_ScheduledCreations.begin(), m_ScheduledCreations.end(), [resource](const Scheduled& scheduled) -> bool {
			return resource == scheduled.resource;
		});

		if (itr == m_ScheduledCreations.end())
		{
			return false;
		}

		// Tell the thread to prioritize this resource creation as we are waiting for it
		m_CreationThread->PrioritizeJob(itr->jobID);

		// Sync until the job is completed
		m_CreationThread->Sync(itr->jobID);

		return true;
	}

	void ResourceManager::ScheduleCreateUploadResource(GpuResource* resource, const char* name, uint64_t size)
	{
		// Is deleted in destructor of ResourceCreationDesc
		wchar_t* wname = new wchar_t[strlen(name) + 1];
		CharToWchar(name, wname);

		ResourceCreationDesc* desc = new ResourceCreationDesc();
		desc->type				 = ResourceType::Upload;
		desc->resource			 = resource;
		desc->name				 = wname;
		desc->size				 = size;

		m_ScheduledCreations.push_back({ resource, m_CreationThread->ScheduleJob(m_CreationJob, desc) });
	}

	void ResourceManager::ScheduleCreateVertexResource(GpuResource* resource, const char* name, uint64_t size)
	{
		// Is deleted in destructor of ResourceCreationDesc
		wchar_t* wname = new wchar_t[strlen(name) + 1];
		CharToWchar(name, wname);

		ResourceCreationDesc* desc = new ResourceCreationDesc();
		desc->type		= ResourceType::Vertex;
		desc->resource	= resource;
		desc->name		= wname;
		desc->size		= size;

		m_ScheduledCreations.push_back({ resource, m_CreationThread->ScheduleJob(m_CreationJob, desc) });
	}

	void ResourceManager::ScheduleCreateIndexResource(GpuResource* resource, const char* name, uint64_t size)
	{
		// Is deleted in destructor of ResourceCreationDesc
		wchar_t* wname = new wchar_t[strlen(name) + 1];
		CharToWchar(name, wname);

		ResourceCreationDesc* desc = new ResourceCreationDesc();
		desc->type		= ResourceType::Index;
		desc->resource	= resource;
		desc->name		= wname;
		desc->size		= size;

		m_ScheduledCreations.push_back({ resource, m_CreationThread->ScheduleJob(m_CreationJob, desc) });
	}

	GPtr<ID3D12Resource> ResourceManager::CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
		D3D12_HEAP_FLAGS heap_flags, D3D12_RESOURCE_STATES start_state, const D3D12_CLEAR_VALUE* clear_value)
	{
		GPtr<ID3D12Resource> resource = nullptr;

		RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(heap_type),
			heap_flags,
			&resource_desc,
			start_state,
			clear_value,
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
		ResourceManager::ResourceCreationDesc* creation_desc = (ResourceManager::ResourceCreationDesc*) data;

		switch (creation_desc->type)
		{
		case ResourceManager::ResourceType::Upload:
		{
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

			creation_desc->resource->SetResource(
				g_ResourceManager->CreateCommittedResource(
					creation_desc->name,
					CD3DX12_RESOURCE_DESC::Buffer(creation_desc->size),
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
					CD3DX12_RESOURCE_DESC::Buffer(creation_desc->size),
					D3D12_HEAP_TYPE_DEFAULT,
					D3D12_HEAP_FLAG_NONE, //D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, // This flag is not allowed for commited resources as they are set automatically
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