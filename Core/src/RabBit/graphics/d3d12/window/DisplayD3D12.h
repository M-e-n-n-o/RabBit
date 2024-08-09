#pragma once
#include "graphics/Display.h"

#include <d3d12.h>
#include <dxgi1_6.h>

namespace RB::Graphics::D3D12
{
	class DisplayD3D12 : public Display
	{
	public:
		DisplayD3D12(GPtr<IDXGIOutput> output, uint32_t output_index);

		const char* GetName() override { return m_Name; }

		Math::Float2 GetResolution() override { return m_Resolution; }

		void* GetNativeHandle() override { return m_Handle; }

	private:
		char				m_Name[128 + 1];
		HMONITOR			m_Handle;
		RB::Math::Float2	m_Resolution;

		//enum Rotation
		//{
		//	kRotation_None = 0,
		//	kRotation_90,
		//	kRotation_180,
		//	kRotation_270
		//};

		//Rotation				m_Rotation;
		//uint32_t				m_BitsPerColor;
		//DXGI_COLOR_SPACE_TYPE	m_ColorSpace;
		//float					m_MinLuminance;
		//float					m_MaxLuminance;
		//float					m_MaxFullscreenLuminance;
	};

	List<Display*> CreateDisplays();
}