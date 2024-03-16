#include "RabBitCommon.h"
#include "CommandList.h"

#include <D3DX12/d3dx12.h>

namespace RB::Graphics::D3D12
{
	CommandList::CommandList(GPtr<ID3D12GraphicsCommandList2> command_list, GPtr<ID3D12CommandAllocator> command_allocator)
		: m_CommandList(command_list)
		, m_CommandAllocator(command_allocator)
	{
		
	}
	
	CommandList::~CommandList()
	{
	}
	
	//void CommandList::UploadBuffer(Buffer* buffer)
	//{
	//	//const void*		data		= buffer->GetData();
	//	//const uint32_t	buffer_size = buffer->GetBufferSize();

	//	//if (data == nullptr)
	//	//{
	//	//	RB_LOG_WARN(LOGTAG_GRAPHICS, "There was no available data inside buffer \"%ls\" to upload", buffer->GetName().c_str());
	//	//	return;
	//	//}
	//	//
	//	//TransitionResourceBarrier(buffer->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST);

	//	//std::wstring upload_name = L"Upload buffer - "; upload_name += buffer->GetName().c_str();
	//	//GPtr<ID3D12Resource> upload_resource =  g_ResourceManager->CreateCommittedResource(
	//	//											upload_name.c_str(),
	//	//											CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
	//	//											D3D12_HEAP_TYPE_UPLOAD,
	//	//											D3D12_HEAP_FLAG_NONE,
	//	//											D3D12_RESOURCE_STATE_GENERIC_READ);

	//	//D3D12_SUBRESOURCE_DATA subresource_data = {};
	//	//subresource_data.pData		= data;
	//	//subresource_data.RowPitch	= buffer_size;
	//	//subresource_data.SlicePitch	= subresource_data.RowPitch;

	//	//UpdateSubresources(m_CommandList.Get(), buffer->GetResource().Get(), upload_resource.Get(), 0, 0, 1, &subresource_data);

	//	//TrackObjectUntilExecuted(upload_resource);
	//	//TrackObjectUntilExecuted(buffer->GetResource());
	//}

	void CommandList::TrackObjectUntilExecuted(GPtr<ID3D12Object> object)
	{
		m_TrackedObjects.push_back(object);
	}

	void CommandList::Reset()
	{
		// Reset both the d3d allocator and the d3d command list
		RB_ASSERT_FATAL_RELEASE_D3D(m_CommandAllocator->Reset(), "Could not reset command allocator");
		RB_ASSERT_FATAL_RELEASE_D3D(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr), "Could not reset command list");

		// Release all temporary d3d objects
		m_TrackedObjects.clear();
	}
}