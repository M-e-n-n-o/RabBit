#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class ResourceManager;

	enum class GpuResourceType
	{
		Texture,
		Buffer,
		Vertex
	};

	class GpuResource
	{
	private:
		GpuResource();
	public:

		// Waits until the actual resource is created and available
		GPtr<ID3D12Resource> GetResource();

		void MarkAsUsed();
		void MarkForDelete();

	private:
		friend class ResourceManager;

		void SetResource(GPtr<ID3D12Resource> resource);

		GPtr<ID3D12Resource> m_Resource;
		GpuResourceType		 m_Type;
	};
}