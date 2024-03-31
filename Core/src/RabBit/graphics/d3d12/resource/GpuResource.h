#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class ResourceManager;

	class GpuResource
	{
	public:
		GpuResource();
		~GpuResource();

		// Should only be called on the render thread!
		GPtr<ID3D12Resource> GetResource();

		void SetResource(GPtr<ID3D12Resource> resource);

		bool IsValid() const;

		void MarkAsUsed();
		void MarkForDelete();

	private:
		friend class ResourceManager;

		GPtr<ID3D12Resource> m_Resource;
	};
}