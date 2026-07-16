#include "EngineEditorWindow.h"
#include "ImGuiManager.h"
#include "app/Application.h"
#include "graphics/Renderer.h"
#include "graphics/Display.h"

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

    EngineEditorWindow::EngineEditorWindow(const char* name)
        : WindowWin(WindowArgs {
                .instance       = GetModuleHandle(nullptr),
                .className      = CharToWString(name),
                .windowName     = name,
                .fullscreen     = false,
                .width          = 1920,
                .height         = 1080,
                .vsync          = true,
                .virtualScale   = 1.0f,
                .virtualAspect  = 0.0f,
                .windowStyle    = kWindowStyle_DraggableBorderless,
                .format         = RenderResourceFormat::RGBA8_UNORM
            })
        , m_Name(name)
    {
        // Make process DPI aware and obtain main monitor scale
        ImGui_ImplWin32_EnableDpiAwareness();

        m_Context = CreateImGuiContext();
        ImGui_ImplWin32_Init(m_WindowHandle);
        InitializeImGuiContextRenderBackend(m_Context, m_BackBufferFormat);

        // Make sure that ImGui gets the windows events before the application itself
        RB::Graphics::Windows::SetOnNativeWindowEventCallback([](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> bool
            {
                auto* window = (EngineEditorWindow*)Application::GetInstance()->FindWindow(hwnd);
                if (window)
                {
                    window->Select();
                    return ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam);
                }
                return false;
            });

        m_TitleWindowName = name;
        m_TitleWindowName += " title bar";
    }

    EngineEditorWindow::~EngineEditorWindow()
    {
        for (int i = 0; i < m_Panels.size(); i++)
        {
            m_Panels[i]->OnDestroy();
            delete m_Panels[i];
        }
    }

    void EngineEditorWindow::Select()
    {
        ImGui::SetCurrentContext(m_Context);
    }

    void EngineEditorWindow::SelectForDraw()
    {
        ImGui::SetNextWindowDockID(m_DockSpaceID, ImGuiCond_FirstUseEver);
    }

    void EngineEditorWindow::DeselectForDraw()
    {
    }

    void EngineEditorWindow::Update()
    {
        // Call base class
        WindowWin::Update();

        ImGui::SetCurrentContext(m_Context);
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Defined in WindowWin
        const int os_title_bar_height = 25;
        const Math::Float4 os_window_rect = GetNativeWindowRectangle();

        // Create the custom title bar
        ImGui::SetNextWindowSize(ImVec2(os_window_rect.x, os_window_rect.y));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin(m_TitleWindowName.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | 
                                                         ImGuiWindowFlags_NoMove | 
                                                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                                                         ImGuiWindowFlags_NoDocking |
                                                         ImGuiWindowFlags_NoScrollWithMouse |
                                                         ImGuiWindowFlags_NoBackground);
        ImGui::SetCursorPos(ImVec2(10, 3));
        ImGui::Text(m_Name);

        ImGui::SameLine(ImGui::GetWindowWidth() - 70);
        if (ImGui::Button("_"))
        {
            ShowWindow(m_WindowHandle, SW_MINIMIZE);
        }
        ImGui::SameLine();
        if (ImGui::Button("O"))
        {
            WindowFullscreenToggleEvent e(m_WindowHandle);
            g_EventManager->InsertEvent(e);
        }
        ImGui::SameLine();
        if (ImGui::Button("X"))
        {
            WindowCloseRequestEvent e(m_WindowHandle);
            g_EventManager->InsertEvent(e);
        }
        ImGui::End();

        // Create the renderable area
        ImGui::SetNextWindowSize(ImVec2(os_window_rect.x, os_window_rect.y - os_title_bar_height));
        ImGui::SetNextWindowPos(ImVec2(0, os_title_bar_height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin(m_Name, nullptr, ImGuiWindowFlags_NoDecoration |
                                      ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoDocking |
                                      ImGuiWindowFlags_NoScrollWithMouse |
                                      ImGuiWindowFlags_NoBackground);

        m_DockSpaceID = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(
            m_DockSpaceID,
            ImVec2(0, 0),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        UpdatePanels();
    }

    void EngineEditorWindow::SetBorderless(bool borderless)
    {
        // Not supported
    }

    void EngineEditorWindow::UpdatePanels()
    {
        Select();
        for (int i = 0; i < m_Panels.size(); i++)
        {
            SelectForDraw();
            m_Panels[i]->OnUpdate();
            DeselectForDraw();
        }
    }

    void EngineEditorWindow::DestroyWindow()
    {
        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        ImGui::SetCurrentContext(m_Context);
        ImGui_ImplWin32_Shutdown();
        DestroyImGuiContext(m_Context);

        // Call base class
        WindowWin::DestroyWindow();
    }
}