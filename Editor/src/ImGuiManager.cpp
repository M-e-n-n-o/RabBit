#include "ImGuiManager.h"
#include "ImGuiManager.h"
#include "backends/imgui_impl_dx12.h"

#include "rabbit/app/Application.h"
#include "rabbit/app/FrameAllocator.h"

#include "platform/windowing/windows/WindowWin.h"
#include "platform/graphics/d3d12/GraphicsDevice.h"
#include "platform/graphics/d3d12/RendererD3D12.h"
#include "platform/graphics/d3d12/DeviceQueue.h"
#include "platform/graphics/d3d12/resource/Descriptor.h"
#include "platform/graphics/d3d12/UtilsD3D12.h"

using namespace RB;
using namespace RB::Graphics;

// Custom per thread ImGui context pointer
thread_local ImGuiContext* g_CustomImGuiTLS;

namespace Editor
{
    UnorderedMap<SIZE_T, D3D12::DescriptorIndex> g_DescriptorState;

    thread_local AllocatorMode g_AllocMode = AllocatorMode::Persistent;

    void* CustomImGuiAllocate(size_t sz, void* user_data)
    {
        if (g_AllocMode == AllocatorMode::Persistent)
        {
            return ALLOC_HEAP(sz);
        } 
        else if (g_AllocMode == AllocatorMode::RenderTransient)
        {
            return Application::GetInstance()->GetRenderer()->GetAllocator()->Allocate(sz);
        }

        RB_LOG_ERROR("ImGui AllocatorMode not implemented");
        return nullptr;
    }

    void CustomImGuiRelease(void* ptr, void* user_data)
    {
        if (g_AllocMode == AllocatorMode::Persistent)
        {
            SAFE_FREE(ptr);
            return;
        }
        else if (g_AllocMode == AllocatorMode::RenderTransient)
        {
            // Memory is auto recycled for the render context
            return;
        }

        RB_LOG_ERROR("ImGui AllocatorMode not implemented");
    }

    void InitializeImGui()
    {
        g_DescriptorState.clear();

        ImGui::SetAllocatorFunctions(CustomImGuiAllocate, CustomImGuiRelease, nullptr);
    }

    void Editor::SetCurrentThreadImGuiAllocatorMode(AllocatorMode mode)
    {
        g_AllocMode = mode;
    }
    
    ImGuiContext* CreateImGuiContext()
    {
        IMGUI_CHECKVERSION();
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // I don't want ImGui to create/destroy windows itself!
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        // Setup style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();
        ImGuiStyle& style = ImGui::GetStyle();

        return ctx;
    }

    void InitializeImGuiContextRenderBackend(ImGuiContext* ctx, RenderResourceFormat format)
    {
        ImGui::SetCurrentContext(ctx);

        ImGui_ImplDX12_InitInfo init_info = {};
        init_info.Device            = D3D12::g_GraphicsDevice->Get().Get();
        init_info.CommandQueue      = D3D12::g_GraphicsDevice->GetGraphicsQueue()->GetCommandQueue().Get();
        init_info.NumFramesInFlight = MAX_NUM_CPU_FRAMES_IN_FLIGHT;
        init_info.RTVFormat         = D3D12::ConvertToDXGIFormat(format);
        init_info.DSVFormat         = DXGI_FORMAT_UNKNOWN;
        init_info.SrvDescriptorHeap = D3D12::g_DescriptorManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap().Get();
        init_info.UserData          = nullptr;
        
        init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
        {
            D3D12::DescriptorIndex di = {};
            di.heapIndex = D3D12::g_DescriptorManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocPersistent();
            di.type      = D3D12::DescriptorHandleType::SRV;
            di.transient = false;

            *out_cpu_desc_handle = D3D12::g_DescriptorManager->GetCpuHandle(di);
            *out_gpu_desc_handle = D3D12::g_DescriptorManager->GetGpuHandle(di);

            g_DescriptorState.emplace(out_cpu_desc_handle->ptr, di);
        };

        init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
        {
            // Find the descriptor index
            auto itr = g_DescriptorState.find(cpu_desc_handle.ptr);
            if (itr == g_DescriptorState.end())
            {
                RB_LOG_ERROR("Could not find allocated ImGUI SRV descriptor back");
                return;
            }

            D3D12::g_DescriptorManager->InvalidateDescriptor(itr->second);
            g_DescriptorState.erase(itr);
        };

        ImGui_ImplDX12_Init(&init_info);
    }
    
    void Editor::DestroyImGuiContext(ImGuiContext* ctx)
    {
        ImGui::SetCurrentContext(ctx);
        ImGui_ImplDX12_Shutdown();
        ImGui::DestroyContext();
    }
}