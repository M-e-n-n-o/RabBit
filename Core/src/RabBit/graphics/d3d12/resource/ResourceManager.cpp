#include "RabBitCommon.h"
#include "../GraphicsDevice.h"
#include "ResourceManager.h"
#include "ResourceStateManager.h"

namespace RB::Graphics::D3D12
{
	// ----------------------------------------------------------------------------
	//							   Resource uploader
	// ----------------------------------------------------------------------------

	//class UploadPage
	//{
	//public:
	//	UploadPage(const wchar_t* name, uint32_t size)
	//		: m_Name(name)
	//		, m_ResourceSize(size)
	//		, m_UploadOffset(0)
	//	{
	//		m_UploadResource = g_ResourceManager->CreateCommittedResource(name, CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ);
	//		m_UploadResource->Map(0, nullptr, (void**)&m_WriteAddress);
	//		m_GpuAddress = m_UploadResource->GetGPUVirtualAddress();
	//	}

	//	void Reset()
	//	{
	//		m_UploadOffset = 0;
	//	}

	//	bool HasSpace(uint32_t size, uint32_t alignment)
	//	{
	//		return Math::AlignUp(Math::AlignUp(m_UploadOffset, alignment) + size, alignment) <= m_ResourceSize;
	//	}

	//	UploadAllocation Allocate(uint32_t size, uint32_t alignment)
	//	{
	//		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, HasSpace(size, alignment), L"Not enough space to allocate %d bytes on upload resource: %ws", size, m_Name);

	//		uint32_t aligned_size = Math::AlignUp(size, alignment);
	//		m_UploadOffset = Math::AlignUp(m_UploadOffset, alignment);

	//		UploadAllocation location = {};
	//		location.entireUploadResource	= m_UploadResource;
	//		location.offsetInResource		= m_UploadOffset;
	//		location.offsetedGpuAddress		= m_GpuAddress + m_UploadOffset;
	//		location.cpuWriteLocation		= m_WriteAddress + aligned_size;
	//		location.maxWriteSize			= aligned_size;

	//		m_UploadOffset += aligned_size;

	//		return location;
	//	}

	//private:
	//	const wchar_t*				m_Name;

	//	GPtr<ID3D12Resource>		m_UploadResource;
	//	D3D12_GPU_VIRTUAL_ADDRESS	m_GpuAddress;
	//	uint8_t*					m_WriteAddress;
	//	uint32_t					m_UploadOffset;
	//	uint32_t					m_ResourceSize;
	//};

	//class ResourceUploader
	//{
	//public:
	//	ResourceUploader(const wchar_t* name, uint32_t page_size)
	//		: m_Name(name)
	//		, m_PageSize(page_size)
	//		, m_CurrentPage(-1)
	//		, m_ResetCounter(0)
	//	{

	//	}

	//	~ResourceUploader()
	//	{
	//		for (int i = 0; i < m_Pages.size(); i++)
	//		{
	//			delete m_Pages[i];
	//		}

	//		m_Pages.clear();
	//	}

	//	void Reset()
	//	{
	//		for (UploadPage* page : m_Pages)
	//		{
	//			page->Reset();
	//		}

	//		m_ResetCounter++;
	//		m_LastHighestPage = Math::Max(m_LastHighestPage, m_CurrentPage);

	//		// Every x resets, check if there are any pages not used, if so, delete them
	//		if (m_ResetCounter == c_DeleteCheck)
	//		{
	//			for (int i = m_LastHighestPage; i < m_Pages.size(); ++i)
	//			{
	//				delete m_Pages[i];
	//			}

	//			m_Pages.resize(Math::Max(m_LastHighestPage, 0));

	//			m_ResetCounter = 0;
	//			m_LastHighestPage = 0;
	//		}
	//		
	//		m_CurrentPage = 0;
	//	}

	//	UploadAllocation Reserve(uint32_t size, uint32_t alignment)
	//	{
	//		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, size <= m_PageSize, L"Upload resource: %ws, is too small to allocate %d bytes, increase the page size", m_Name, size);

	//		if (m_CurrentPage == -1)
	//		{
	//			m_CurrentPage = AddNewPage();
	//		}

	//		if (!m_Pages[m_CurrentPage]->HasSpace(size, alignment))
	//		{
	//			// Check if other pages have space
	//			bool found_space = false;
	//			for (int i = 0; i < m_Pages.size(); ++i)
	//			{
	//				if (m_Pages[i]->HasSpace(size, alignment))
	//				{
	//					found_space = true;

	//					// Make sure to keep track of which pages were used
	//					m_LastHighestPage = Math::Max(m_LastHighestPage, m_CurrentPage);

	//					m_CurrentPage = i;
	//					break;
	//				}
	//			}

	//			if (!found_space)
	//			{
	//				m_CurrentPage = AddNewPage();
	//			}
	//		}

	//		return m_Pages[m_CurrentPage]->Allocate(size, alignment);
	//	}

	//private:
	//	int32_t AddNewPage()
	//	{
	//		std::wstring name(m_Name);
	//		name += L" page ";
	//		name += m_Pages.size();

	//		m_Pages.push_back(new UploadPage(name.c_str(), m_PageSize));

	//		return m_Pages.size() - 1;
	//	}

	//	const wchar_t*		m_Name;

	//	List<UploadPage*>	m_Pages;
	//	int32_t				m_CurrentPage;
	//	
	//	uint32_t			m_PageSize;

	//	const uint32_t		c_DeleteCheck = 10;
	//	uint32_t			m_ResetCounter;
	//	int32_t				m_LastHighestPage;
	//};

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

	ResourceManager* g_ResourceManager = nullptr;

	ResourceManager::ResourceManager()
	{

	}

	ResourceManager::~ResourceManager()
	{
		// Wait until all objects can be release
		for (auto itr = m_ObjsWaitingToFinishFlight.begin(); itr != m_ObjsWaitingToFinishFlight.end();)
		{
			itr->first->queue->CpuWaitForFenceValue(itr->first->fenceValue);
			
			delete itr->first;
			itr = m_ObjsWaitingToFinishFlight.erase(itr);
		}
	}

	void ResourceManager::StartFrame()
	{

	}

	void ResourceManager::EndFrame()
	{
		// Check which tracked resources can be deleted
		for (auto itr = m_ObjsWaitingToFinishFlight.begin(); itr != m_ObjsWaitingToFinishFlight.end();)
		{
			if (itr->first->queue->IsFenceReached(itr->first->fenceValue))
			{
				delete itr->first;
				itr = m_ObjsWaitingToFinishFlight.erase(itr);
			}
			else
			{
				++itr;
			}
		}
	}

	void ResourceManager::MarkForDelete(GpuResource* resource)
	{
		if (resource->m_Resource)
		{
			m_ObjsScheduledToReleaseAfterExecute.push_back(resource->m_Resource);
		}
	}

	void ResourceManager::OnCommandListExecute(DeviceQueue* queue, uint64_t fence_value)
	{
		FencePair* fence_pair = new FencePair{ queue, fence_value };

		m_ObjsWaitingToFinishFlight.emplace(fence_pair, m_ObjsScheduledToReleaseAfterExecute);
		m_ObjsScheduledToReleaseAfterExecute.clear();
	}

	bool ResourceManager::CreateUploadResource(GpuResource* out_resource, const wchar_t* name, uint64_t size)
	{
		// TODO: Make this deferred and create and upload the actual resource and resource data on a different thread
		out_resource->SetResource(CreateCommittedResource(name, CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ));
		
		// TODO Return if resource creation succeeded
		return true;
	}

	bool ResourceManager::CreateVertexResource(GpuResource* out_resource, const wchar_t* name, uint64_t size)
	{
		// TODO: Make this deferred and create and upload the actual resource and resource data on a different thread
		out_resource->SetResource(CreateCommittedResource(name, CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_RESOURCE_STATE_COPY_DEST));
		
		// TODO Return if resource creation succeeded
		return true;
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

			// Start tracking resources state
			g_ResourceStateManager->StartTrackingResource(resource.Get(), start_state);
		}

		return resource;
	}
}