#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class DeviceQueue;

	class GpuResource
	{
	public:
		GpuResource(std::function<void(GpuResource*)> on_resource_created_callback = nullptr);
		GpuResource(GPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state, bool transfer_ownership);
		~GpuResource();

		// Should only be called on the render thread!
		GPtr<ID3D12Resource> GetResource();

		void SetResource(GPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state);

		bool IsValid() const;

		void MarkAsUsed(DeviceQueue* queue);

		void UpdateState(D3D12_RESOURCE_STATES state);
		D3D12_RESOURCE_STATES GetState() const;
		bool IsInState(D3D12_RESOURCE_STATES state) const;

	private:
		GPtr<ID3D12Resource>				m_Resource;
		D3D12_RESOURCE_STATES				m_State;
		bool								m_OwnsResource;
		std::function<void(GpuResource*)>	m_OnCreationCallback;
	};
}