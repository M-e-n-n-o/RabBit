#if RB_PLATFORM_WINDOWS && RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "DisplayWin.h"
#include "platform/graphics/d3d12/GraphicsDevice.h"

#include <strsafe.h>

using namespace RB::Graphics::D3D12;

namespace RB::Graphics::Windows
{
    List<Display*> g_Displays;

    DisplayWin::DisplayWin(HMONITOR monitor_handle, const char name[128], RB::Math::Float2 resolution)
        : m_Handle(monitor_handle)
        , m_Resolution(resolution)
    {
        StringCchCopy(m_Name, _countof(m_Name), name);
    }

    BOOL CALLBACK MonitorEnumProc(HMONITOR h_monitor, HDC hdc_monitor, LPRECT lprc_monitor, LPARAM dw_data) 
    {
        MONITORINFOEX monitor_info;
        monitor_info.cbSize = sizeof(monitor_info);
        if (GetMonitorInfo(h_monitor, &monitor_info))
        {
            DISPLAY_DEVICE display_device;
            display_device.cb = sizeof(display_device);

            // Query for display device matching monitor name
            if (EnumDisplayDevices(monitor_info.szDevice, 0, &display_device, 0)) 
            {
                Display* display = new DisplayWin(h_monitor, 
                                                  display_device.DeviceString, 
                                                  RB::Math::Float2(monitor_info.rcMonitor.right - monitor_info.rcMonitor.left, monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top));

                g_Displays.push_back(display);
            }
        }
        return TRUE;
    }

    List<Display*> CreateDisplays()
    {
        if (g_Displays.empty())
        {
            EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);
        }

        return g_Displays;
    }
}
#endif