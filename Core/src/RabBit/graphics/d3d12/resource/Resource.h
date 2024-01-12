#pragma once

#include "RabBitCommon.h"
#include "math/Vector.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class Resource
	{
	public:
		void Map();
		void UnMap();

	protected:
		bool						m_InFlight;
		bool						m_HasBeenMarkedForDelete;

		std::wstring				m_Name;
		GPtr<ID3D12Resource>		m_Resource;
		Unique<D3D12_CLEAR_VALUE>	m_ClearValue;
		bool						m_IsCPUAccessible;
	};

	class ResourceDescriptor
	{

	};

	class ResourceHolder
	{
	private:
		Resource*			m_Resource;
		ResourceDescriptor*	m_Descriptor;
	};

	class Texture : public ResourceHolder
	{

	};

	class Buffer : public ResourceHolder
	{

	};
}