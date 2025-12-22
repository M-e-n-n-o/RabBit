#include <RabBit.h>

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;

//#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

class Editor : public RB::Application
{
public:
    Editor(RB::AppInfo& info) : Application(info) {}

    void OnStart() override
    {
        // Make process DPI aware and obtain main monitor scale
        ImGui_ImplWin32_EnableDpiAwareness();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        ImGuiStyle& style = ImGui::GetStyle();

        // Setup Platform/Renderer backends
        //ImGui_ImplWin32_Init(hwnd);
        //GetWindow
    }

    void OnStop() override
    {
    }
};

RB::Application* RB::CreateApplication(const char* launch_args)
{
    AppInfo app_info = {};
    app_info.appName = "RabBit Editor";

    AppInfo::Window window = {};
    window.windowName          = "RabBit Editor";
    window.fullscreen          = false;
    window.windowIndex         = 0;
    window.vsync               = false;
    window.windowWidth         = 1280;
    window.windowHeight        = 720;
    app_info.windows.push_back(window);

    return new Editor(app_info);
}