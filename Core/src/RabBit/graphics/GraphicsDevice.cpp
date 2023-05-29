#include "RabBitPch.h"
#include "GraphicsDevice.h"

#ifdef RB_CONFIG_DEBUG
#include <dxgidebug.h>
#endif

namespace RB::Graphics
{
	GraphicsDevice* g_GraphicsDevice = nullptr;

	void ReportLiveObjects()
	{
		IDXGIDebug1* debug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));

		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
		debug->Release();
	}

	GraphicsDevice::GraphicsDevice()
	{
#ifdef RB_CONFIG_DEBUG
		GPtr<ID3D12Debug> debug_interface;
		RB_ASSERT_FATAL_D3D(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)), "Could not get the debug interface");
		debug_interface->EnableDebugLayer();
#endif

		CreateAdapter();
		CreateDevice();
	}

	GraphicsDevice::~GraphicsDevice()
	{
#ifdef RB_CONFIG_DEBUG
		RB_LOG("Outputting live objects to VS console...");
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

		GPtr<IDXGIFactory4> factory4;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
		{
			GPtr<IDXGIFactory5> factory5;
			if (SUCCEEDED(factory4.As(&factory5)))
			{
				if (FAILED(factory5->CheckFeatureSupport(feature, &supported, sizeof(supported))))
				{
					supported = FALSE;
				}
			}
		}

		return supported == TRUE;
	}

	void GraphicsDevice::CreateAdapter()
	{
		GPtr<IDXGIFactory4> dxgi_factory;
		uint32_t factory_flags = 0;
#ifdef RB_CONFIG_DEBUG
		factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		RB_ASSERT_FATAL_RELEASE_D3D(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&dxgi_factory)), "Could not create factory");

		GPtr<IDXGIAdapter1> dxgi_adapter1;

		RB_LOG("Found the following D3D12 compatible GPU's:");

		std::string desc_best_adapter;
		int64_t max_dedicated_vram = 0;
		for (uint32_t adapter_index = 0; dxgi_factory->EnumAdapters1(adapter_index, &dxgi_adapter1) != DXGI_ERROR_NOT_FOUND; adapter_index++)
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
				RB_LOG("\t%d. %s", adapter_index + 1, desc);

				if (dxgi_adapter_desc.DedicatedVideoMemory > max_dedicated_vram)
				{
					max_dedicated_vram = dxgi_adapter_desc.DedicatedVideoMemory;
					desc_best_adapter = desc;
					RB_ASSERT_FATAL_RELEASE_D3D(dxgi_adapter1.As(&m_NativeAdapter), "Could not set adapter");
				}
			}
		}

		RB_ASSERT_FATAL_RELEASE_D3D(max_dedicated_vram != 0, "Could not find any D3D12 compatible GPU");

		RB_LOG("Selected graphics device:");
		RB_LOG("\tName: %s", desc_best_adapter.c_str());
		RB_LOG("\tVRAM: %d MB", max_dedicated_vram / 1000000);
	}

	void GraphicsDevice::CreateDevice()
	{
		RB_ASSERT_FATAL_RELEASE_D3D(D3D12CreateDevice(m_NativeAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_NativeDevice)), "Could not create D3D12 device");

		// Enable debug messages
#ifdef RB_CONFIG_DEBUG
		GPtr<ID3D12InfoQueue> info_queue;
		if (FAILED(m_NativeDevice.As(&info_queue)))
		{
			RB_LOG_ERROR("Could not create D3D12 info queue, D3D12 debug messages will not be visible");
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
}