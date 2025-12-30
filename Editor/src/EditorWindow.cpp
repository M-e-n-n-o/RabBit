#include "EditorWindow.h"
#include "ImGuiManager.h"
#include "app/Application.h"
#include "graphics/Renderer.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

using namespace RB;
using namespace RB::Events;
using namespace RB::Graphics;
using namespace RB::Graphics::Windows;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Editor
{
    // Window callback function
    LRESULT CALLBACK WindowCallback(HWND, UINT, WPARAM, LPARAM);

    EditorWindow::EditorWindow(const char* name)
        : WindowWin(WindowArgs {
                .instance       = GetModuleHandle(nullptr),
                .className      = CharToWString(name),
                .windowName     = name,
                .fullscreen     = false,
                .width          = 1280,
                .height         = 720,
                .vsync          = true,
                .virtualScale   = 1.0f,
                .virtualAspect  = 0.0f,
                .windowStyle    = kWindowStyle_DraggableBorderless,
                .format         = RenderResourceFormat::R8G8B8A8_UNORM
            })
    {
        // Make process DPI aware and obtain main monitor scale
        ImGui_ImplWin32_EnableDpiAwareness();

        // Make sure that ImGui gets the windows events before the application itself
        RB::Graphics::Windows::SetOnNativeWindowEventCallback([](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> bool
            {
                return ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam);
            });

        m_Context = CreateImGuiContext();
        ImGui_ImplWin32_Init(m_WindowHandle);
        InitializeImGuiContextRenderBackend(m_Context, m_BackBufferFormat);
    }

    EditorWindow::~EditorWindow()
    {
    }

    void EditorWindow::Select()
    {
        ImGui::SetCurrentContext(m_Context);
    }

    void EditorWindow::PrepareDraw()
    {
        ImGui::SetCurrentContext(m_Context);
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void EditorWindow::SetBorderless(bool borderless)
    {
        // Not supported
    }

    void EditorWindow::DestroyWindow()
    {
        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        ImGui::SetCurrentContext(m_Context);
        ImGui_ImplWin32_Shutdown();
        DestroyImGuiContext(m_Context);

        // Call base class
        WindowWin::DestroyWindow();
    }
}