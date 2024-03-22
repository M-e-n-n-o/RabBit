#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "ResourceManager.h"
#include <D3DX12/d3dx12.h>

namespace RB::Graphics::D3D12
{
	VertexBufferD3D12::VertexBufferD3D12(const char* name, void* data, uint64_t data_size)
		: m_Name(name)
		, m_Size(data_size)
	{
		if (!data)
		{
			return;
		}

		wchar_t* res_name = (wchar_t*)ALLOC_STACK(sizeof(wchar_t) * strlen(m_Name) + 1);
		CharToWchar(m_Name, res_name);

		m_Resource = g_ResourceManager->CreateTexture2D(res_name, data_size, data);

		// TODO add Just-In-Time upload (so only upload the data onto the upload resource and copy it 
		// over into the actual resource when it is being rendered for the first time.
		// Later, we need to do resource creation on a different thread

		// TODO 
		// - Hoe kom ik nu bij de commandlist om een copy resource te doen vanaf de upload resource naar de echte resource?
		// - Als de copy is gecalled op de commandlist moet ik op die commandlist ook de upload resource gaan tracken totdat de
		//   commandlist is geexecute (wachten totdat de fence value is geraakt).
	}
}