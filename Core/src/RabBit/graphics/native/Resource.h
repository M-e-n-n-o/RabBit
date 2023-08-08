#pragma once

#include "RabBitCommon.h"
#include "math/Vector.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	class Resource
	{
	public:
		Resource(const wchar_t* name = L"");
		Resource(const D3D12_RESOURCE_DESC& resource_desc, const D3D12_CLEAR_VALUE* clear_value = nullptr, const wchar_t* name = L"");
		Resource(GPtr<ID3D12Resource> resource, const wchar_t* name = L"");
		Resource(const Resource& copy);
		Resource(Resource&& copy);

		Resource& operator=(const Resource& other);
		Resource& operator=(Resource&& other);

		virtual ~Resource();

		bool IsValid() const;

		// No need to Unmap
		// Set range to -1 to specify the whole resource might be written to
		bool Map(void** mapped_memory, Math::Int2 range = Math::Int2(-1), uint32_t subresource_index = 0);

		D3D12_RESOURCE_DESC GetDesc() const;

		virtual void Reset();

		// Replace the resource
		virtual void SetResource(GPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clear_value = nullptr);

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC* srv_desc = nullptr) const = 0;

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav_desc = nullptr) const = 0;

	protected:
		std::wstring				m_Name;
		GPtr<ID3D12Resource>		m_Resource;
		Unique<D3D12_CLEAR_VALUE>	m_ClearValue;
	};
}