#include "RabBitPch.h"
#include "Resource.h"
#include "ResourceManager.h"

#include <D3DX12/d3dx12.h>

namespace RB::Graphics::Native
{
	Resource::Resource(const wchar_t* name)
		: m_Name(name)
	{
	}

	Resource::Resource(const D3D12_RESOURCE_DESC& resource_desc, const D3D12_CLEAR_VALUE* clear_value, const wchar_t* name)
		: m_Name(name)
	{
		if (clear_value)
		{
			m_ClearValue = CreateUnique<D3D12_CLEAR_VALUE>(*clear_value);
		}

		m_Resource = g_ResourceManager->CreateCommittedResource(m_Name.c_str(), resource_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, clear_value);
	}

	Resource::Resource(GPtr<ID3D12Resource> resource, const wchar_t* name)
		: m_Name(name)
		, m_Resource(resource)
	{
		m_Resource->SetName(m_Name.c_str());
	}

	Resource::Resource(const Resource& copy)
		: m_Name(copy.m_Name)
		, m_Resource(copy.m_Resource)
		, m_ClearValue(CreateUnique<D3D12_CLEAR_VALUE>(*copy.m_ClearValue))
	{
	}

	Resource::Resource(Resource&& copy)
		: m_Name(std::move(copy.m_Name))
		, m_Resource(std::move(copy.m_Resource))
		, m_ClearValue(std::move(copy.m_ClearValue))
	{
	}

	Resource& Resource::operator=(const Resource& other)
	{
		if (this != &other)
		{
			m_Resource = other.m_Resource;
			m_Name = other.m_Name;

			if (other.m_ClearValue)
			{
				m_ClearValue = CreateUnique<D3D12_CLEAR_VALUE>(*other.m_ClearValue);
			}
		}

		return *this;
	}

	Resource& Resource::operator=(Resource&& other)
	{
		if (this != &other)
		{
			m_Name		 = other.m_Name;
			m_Resource	 = other.m_Resource;
			m_ClearValue = std::move(other.m_ClearValue);

			other.m_Resource.Reset();
			other.m_Name.clear();
		}

		return *this;
	}

	Resource::~Resource()
	{
	}

	bool Resource::IsValid() const
	{
		return (m_Resource != nullptr);
	}

	bool Resource::Map(void** mapped_memory, Math::Int2 range, uint32_t subresource_index)
	{
		CD3DX12_RANGE* d3d_range = (CD3DX12_RANGE*) alloca(sizeof(CD3DX12_RANGE));

		d3d_range->Begin = range.x;
		d3d_range->End	 = range.y;	

		if (range.x == -1 || range.y == -1)
		{
			d3d_range = nullptr;
		}

		HRESULT result = m_Resource->Map(subresource_index, d3d_range, mapped_memory);

		return SUCCEEDED(result);
	}

	D3D12_RESOURCE_DESC Resource::GetDesc() const
	{
		D3D12_RESOURCE_DESC desc = {};

		if (m_Resource)
		{
			desc = m_Resource->GetDesc();
		}

		return desc;
	}

	void Resource::Reset()
	{
		m_Resource.Reset();
		m_ClearValue.reset();
	}

	void Resource::SetResource(GPtr<ID3D12Resource> new_resource, const D3D12_CLEAR_VALUE* clear_value)
	{
		m_Resource = new_resource;

		if (m_ClearValue)
		{
			m_ClearValue = CreateUnique<D3D12_CLEAR_VALUE>(*clear_value);
		}
		else
		{
			m_ClearValue.reset();
		}

		m_Resource->SetName(m_Name.c_str());
	}
}