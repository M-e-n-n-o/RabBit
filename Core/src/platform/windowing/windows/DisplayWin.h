#if RB_PLATFORM_WINDOWS

#pragma once
#include "math/Vector.h"
#include "graphics/Display.h"

namespace RB::Graphics::Windows
{
    class DisplayWin : public Display
    {
    public:
        DisplayWin(HMONITOR monitor_handle, const char name[128], RB::Math::Float2 resolution);

        const char* GetName() override { return m_Name; }

        Math::Float2 GetResolution() override { return m_Resolution; }

        void* GetNativeHandle() override { return m_Handle; }

    private:
        char		        m_Name[128];
        HMONITOR            m_Handle;
        RB::Math::Float2    m_Resolution;

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
#endif