#include "RabBitCommon.h"
#include "GraphicsDevice.h"
#include "DeviceQueue.h"

#ifdef RB_CONFIG_DEBUG
#include <dxgidebug.h>
#endif

#include <d3d11.h>
#include <d3d11on12.h>

#include <strsafe.h>

// Agility SDK constants (update the version number if the SDK version is ever upgraded!)
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 613; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

namespace RB::Graphics::D3D12
{
	GraphicsDevice* g_GraphicsDevice = nullptr;

#ifdef RB_CONFIG_DEBUG
	void ReportLiveObjects()
	{
		IDXGIDebug1* debug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));

		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}
#endif

	GraphicsDevice::GraphicsDevice(bool enable_debug_layer)
		: m_CopyQueue(nullptr)
		, m_ComputeQueue(nullptr)
		, m_GraphicsQueue(nullptr)
	{
		// Tell Windows that this thread is DPI aware so it does not automatically apply scaling
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if !defined(RB_CONFIG_DIST)
		if (enable_debug_layer)
		{
			GPtr<ID3D12Debug1> debug_interface;
			RB_ASSERT_FATAL_D3D(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)), "Could not get the debug interface");
			debug_interface->EnableDebugLayer();

			// Enable these for full validation (very slow)
			//debug_interface->SetEnableGPUBasedValidation(TRUE);
			//debug_interface->SetEnableSynchronizedCommandQueueValidation(TRUE);
		}
#endif

		CreateFactory();
		RB_LOG(LOGTAG_GRAPHICS, "-------- ADAPTER INFORMATION --------");
		CreateAdapter();
		RB_LOG(LOGTAG_GRAPHICS, "-------- MONITOR INFORMATION --------");
		CreateMonitors();
		CreateDevice(enable_debug_layer);
	}

	GraphicsDevice::~GraphicsDevice()
	{
		SAFE_DELETE(m_CopyQueue);
		SAFE_DELETE(m_ComputeQueue);
		SAFE_DELETE(m_GraphicsQueue);

#ifdef RB_CONFIG_DEBUG
		RB_LOG(LOGTAG_GRAPHICS, "Outputting live objects to console...");
		atexit(&ReportLiveObjects);
#endif
	}

	bool GraphicsDevice::IsFormatSupported(DXGI_FORMAT format)
	{
		D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support = { format };
		if (FAILED(m_NativeDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &format_support, sizeof(format_support))))
		{
			return false;
		}

		return true;
	}

	bool GraphicsDevice::IsFeatureSupported(DXGI_FEATURE feature)
	{
		BOOL supported = FALSE;

		GPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(m_Factory.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(feature, &supported, sizeof(supported))))
			{
				supported = FALSE;
			}
		}

		return supported == TRUE;
	}

	void GraphicsDevice::WaitUntilIdle()
	{
		if (m_CopyQueue)
		{
			m_CopyQueue->CpuWaitUntilIdle();
		}

		if (m_ComputeQueue)
		{
			m_ComputeQueue->CpuWaitUntilIdle();
		}

		if (m_GraphicsQueue)
		{
			m_GraphicsQueue->CpuWaitUntilIdle();
		}
	}

	DeviceQueue* GraphicsDevice::GetCopyQueue()
	{
		if (!m_CopyQueue)
		{
			m_CopyQueue = new DeviceQueue(D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE);
		}

		return m_CopyQueue;
	}

	DeviceQueue* GraphicsDevice::GetComputeQueue()
	{
		if (!m_ComputeQueue)
		{
			m_ComputeQueue = new DeviceQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE);
		}

		return m_ComputeQueue;
	}

	DeviceQueue* GraphicsDevice::GetGraphicsQueue()
	{
		if (!m_GraphicsQueue)
		{
			m_GraphicsQueue = new DeviceQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_HIGH, D3D12_COMMAND_QUEUE_FLAG_NONE);
		}

		return m_GraphicsQueue;
	}

	void GraphicsDevice::CreateFactory()
	{
		uint32_t factory_flags = 0;
#ifdef RB_CONFIG_DEBUG
		factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		RB_ASSERT_FATAL_RELEASE_D3D(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&m_Factory)), "Could not create factory");
	}

	void GraphicsDevice::CreateAdapter()
	{
		GPtr<IDXGIAdapter1> dxgi_adapter1;

		RB_LOG(LOGTAG_GRAPHICS, "Found the following D3D12 compatible GPU's:");

		std::string desc_best_adapter;
		int64_t max_dedicated_vram = 0;
		int64_t max_shared_smem = 0;
		for (uint32_t adapter_index = 0; m_Factory->EnumAdapters1(adapter_index, &dxgi_adapter1) != DXGI_ERROR_NOT_FOUND; adapter_index++)
		{
			DXGI_ADAPTER_DESC1 dxgi_adapter_desc;
			dxgi_adapter1->GetDesc1(&dxgi_adapter_desc);

			// Check if the adapter can create a D3D12 device, adapter with largest dedicated video memory is favored
			if ((dxgi_adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgi_adapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				char desc[128];
				char def_char = ' ';
				WideCharToMultiByte(CP_ACP, 0, dxgi_adapter_desc.Description, -1, desc, 128, &def_char, NULL);
				//output << "\t\t" << (adapter_index + 1) << ". " << desc << "\n";
				RB_LOG(LOGTAG_GRAPHICS, "\t%d. %s", adapter_index + 1, desc);

				if (dxgi_adapter_desc.DedicatedVideoMemory > max_dedicated_vram)
				{
					max_dedicated_vram = dxgi_adapter_desc.DedicatedVideoMemory;
					max_shared_smem = dxgi_adapter_desc.SharedSystemMemory;
					desc_best_adapter = desc;
					RB_ASSERT_FATAL_RELEASE_D3D(dxgi_adapter1.As(&m_NativeAdapter), "Could not set adapter");
				}
			}
		}

		RB_ASSERT_FATAL_RELEASE_D3D(max_dedicated_vram != 0, "Could not find any D3D12 compatible GPU");

		RB_LOG(LOGTAG_GRAPHICS, "Selected graphics device:");
		RB_LOG(LOGTAG_GRAPHICS, "\tName: %s", desc_best_adapter.c_str());
		RB_LOG(LOGTAG_GRAPHICS, "\tVRAM: %d MB", max_dedicated_vram / 1000000);

		m_GpuInfo						= {};
		m_GpuInfo.name					= desc_best_adapter.c_str();
		m_GpuInfo.videoMemory			= max_dedicated_vram;
		m_GpuInfo.sharedSystemMemory	= max_shared_smem;
	}

	void GraphicsDevice::CreateMonitors()
	{
		// To get all possible fullscreen resolutions, formats and refresh rate see:
		// https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutput1-getdisplaymodelist1

		RB_LOG(LOGTAG_GRAPHICS, "Found the following monitors: ");

		GPtr<IDXGIOutput> output;
		for (uint32_t output_index = 0; m_NativeAdapter->EnumOutputs(output_index, &output) != DXGI_ERROR_NOT_FOUND; output_index++)
		{
			GPtr<IDXGIOutput6> output6;
			RB_ASSERT_FATAL_RELEASE_D3D(output.As(&output6), "Could not query output 6");

			DXGI_OUTPUT_DESC1 desc;
			RB_ASSERT_FATAL_RELEASE_D3D(output6->GetDesc1(&desc), "Could not retrieve description for IDXGIOutput");

			MonitorInfo info			= {};
			info.resolution				= Math::Float2(desc.DesktopCoordinates.right - desc.DesktopCoordinates.left, desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
			info.rotation				= MonitorInfo::Rotation(Math::Abs(desc.Rotation - 1));
			info.bitsPerColor			= desc.BitsPerColor;
			info.colorSpace				= desc.ColorSpace;
			info.minLuminance			= desc.MinLuminance;
			info.maxLuminance			= desc.MaxLuminance;
			info.maxFullscreenLuminance = desc.MaxFullFrameLuminance;
			info.name					= new char[128 + 1];

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
					RB_ASSERT_FATAL_RELEASE_D3D(StringCchCopy(sz_save_device_name, 33, disp_dev.DeviceName), "Failed to string copy");

					// After second call, disp_dev.DeviceString is the  
					// monitor name for that device
					EnumDisplayDevices(sz_save_device_name, 0, &disp_dev, 0);

					// In the following, lpszMonitorInfo must be 128 + 1 for  
					// the null-terminator.
					RB_ASSERT_FATAL_RELEASE_D3D(StringCchCopy(info.name, 129, disp_dev.DeviceString), "Failed to string copy");
				}
				else 
				{
					RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Failed to enum display devices")
				}
			}

			m_Monitors.push_back(info);

			RB_LOG(LOGTAG_GRAPHICS, "\t%d. %s (%d x %d)", (int) output_index, info.name, (int)info.resolution.x, (int)info.resolution.y);
		}
	}

	void GraphicsDevice::CreateDevice(bool enable_debug_messages)
	{
		RB_ASSERT_FATAL_RELEASE_D3D(D3D12CreateDevice(m_NativeAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_NativeDevice)), "Could not create D3D12 device");

		// Enable debug messages
#if !defined(RB_CONFIG_DIST)
		if (!enable_debug_messages)
		{
			return;
		}

		GPtr<ID3D12InfoQueue> info_queue;
		if (FAILED(m_NativeDevice.As(&info_queue)))
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Could not create D3D12 info queue, D3D12 debug messages will not be visible");
			return;
		}

		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,	TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,		TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,		TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO,			FALSE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE,		FALSE);

		// Suppress whole categories of messages
		// D3D12_MESSAGE_CATEGORY categories[] = 
		// {
		//
		// };

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID ids[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		//filter.DenyList.NumCategories = _countof(categories);
		//filter.DenyList.pCategoryList = categories;
		filter.DenyList.NumSeverities	= _countof(severities);
		filter.DenyList.pSeverityList	= severities;
		filter.DenyList.NumIDs			= _countof(ids);
		filter.DenyList.pIDList			= ids;

		RB_ASSERT_FATAL_RELEASE_D3D(info_queue->PushStorageFilter(&filter), "Could not set the D3D12 message filter");
#endif
	}

	GPtr<ID3D11On12Device> GraphicsDevice::Get11On12()
	{
		GPtr<ID3D11Device> d3d11Device;

		RB_ASSERT_FATAL_RELEASE_D3D(D3D11On12CreateDevice(
			m_NativeDevice.Get(),
			D3D11_CREATE_DEVICE_BGRA_SUPPORT, // Needed for Direct Composition
			NULL,
			0,
			reinterpret_cast<IUnknown**>(GetGraphicsQueue()->GetCommandQueue().GetAddressOf()),
			1,
			0,
			&d3d11Device,
			&m_11DeviceContext,
			nullptr
		), "Could not create a 11 on 12 device");

		RB_ASSERT_FATAL_RELEASE_D3D(d3d11Device.As(&m_11On12Device), "Could not query the 11 on 12 device");

		return m_11On12Device;
	}
}