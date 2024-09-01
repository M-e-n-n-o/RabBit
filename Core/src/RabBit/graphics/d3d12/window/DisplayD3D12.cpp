#include "RabBitCommon.h"
#include "DisplayD3D12.h"
#include "graphics/d3d12/GraphicsDevice.h"

#include <strsafe.h>

namespace RB::Graphics::D3D12
{
    DisplayD3D12::DisplayD3D12(GPtr<IDXGIOutput> output, uint32_t output_index)
    {
        // To get all possible fullscreen resolutions, formats and refresh rate see:
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutput1-getdisplaymodelist1

        GPtr<IDXGIOutput6> output6;
        RB_ASSERT_FATAL_RELEASE_D3D(output.As(&output6), "Could not query output 6");

        DXGI_OUTPUT_DESC1 desc;
        RB_ASSERT_FATAL_RELEASE_D3D(output6->GetDesc1(&desc), "Could not retrieve description for IDXGIOutput");

        //MonitorInfo info				= {};
        //info.rotation					= MonitorInfo::Rotation(Math::Abs(desc.Rotation - 1));
        //info.bitsPerColor				= desc.BitsPerColor;
        //info.colorSpace				= desc.ColorSpace;
        //info.minLuminance				= desc.MinLuminance;
        //info.maxLuminance				= desc.MaxLuminance;
        //info.maxFullscreenLuminance	= desc.MaxFullFrameLuminance;

        m_Handle = desc.Monitor;

        m_Resolution = Math::Float2(desc.DesktopCoordinates.right - desc.DesktopCoordinates.left, desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);

        // Retrieve monitor's friendly name
        {
            DISPLAY_DEVICE disp_dev;
            char sz_save_device_name[33];

            ZeroMemory(&disp_dev, sizeof(disp_dev));
            disp_dev.cb = sizeof(disp_dev);

            // After the first call to EnumDisplayDevices,  
            // disp_dev.DeviceString is the adapter name
            if (EnumDisplayDevices(NULL, output_index, &disp_dev, 0))
            {
                RB_ASSERT_FATAL_RELEASE_D3D(StringCchCopy(sz_save_device_name, _countof(sz_save_device_name), disp_dev.DeviceName), "Failed to string copy");

                // After second call, disp_dev.DeviceString is the  
                // monitor name for that device
                EnumDisplayDevices(sz_save_device_name, 0, &disp_dev, 0);

                // In the following, lpszMonitorInfo must be 128 + 1 for  
                // the null-terminator.
                RB_ASSERT_FATAL_RELEASE_D3D(StringCchCopy(m_Name, _countof(m_Name), disp_dev.DeviceString), "Failed to string copy");
            }
            else
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Failed to enum display devices")
            }
        }
    }

    List<Display*> CreateDisplays()
    {
        List<Display*> displays;

        GPtr<IDXGIOutput> output;
        for (uint32_t output_index = 0; g_GraphicsDevice->GetAdapter()->EnumOutputs(output_index, &output) != DXGI_ERROR_NOT_FOUND; output_index++)
        {
            Display* display = new DisplayD3D12(output, output_index);
            displays.push_back(display);
        }

        return displays;
    }
}
