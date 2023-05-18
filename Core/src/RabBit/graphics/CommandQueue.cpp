#include "RabBitPch.h"
#include "CommandQueue.h"
#include "GraphicsDevice.h"

namespace RB::Graphics
{
	CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags)
		: CommandQueue(g_GraphicsDevice, type, priority, flags)
	{
	}

	CommandQueue::CommandQueue(GPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type		= type;
		desc.Priority	= priority;
		desc.Flags		= flags;
		desc.NodeMask	= 0;

		RB_ASSERT_FATAL_RELEASE_D3D(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_NativeCommandQueue)), "Could not create command queue");
	}

	CommandQueue::~CommandQueue()
	{
	}
}