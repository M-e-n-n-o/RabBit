#include "RabBitCommon.h"
#include "Application.h"
#include "AssetManager.h"
#include "PlatformService.h"

#include "graphics/Window.h"
#include "graphics/RenderInterface.h"
#include "graphics/Renderer.h"
#include "graphics/Display.h"

#include "entity/Scene.h"

#include "events/ApplicationEvent.h"
#include "events/KeyEvent.h"
#include "events/input/KeyCodes.h"
#include "events/input/Input.h"

#include "utils/Timer.h"

using namespace RB::Graphics;
using namespace RB::Events;
using namespace RB::Entity;

namespace RB
{
    Application* Application::s_Instance = nullptr;

    Application::Application(AppInfo& info)
        : EventListener(kEventCat_All)
        , m_Initialized(false)
        , m_ShouldStop(false)
        , m_CheckWindows(false)
        , m_PrimaryWindowIndex(0)
        , m_FrameIndex(0)
        , m_DeltaTime(0)
        , m_FixedTimeStep(-1)
        , m_PlatformService(nullptr)
    {
        RB_ASSERT_FATAL(LOGTAG_MAIN, s_Instance == nullptr, "Application already exists");
        s_Instance = this;

        m_StartAppInfo = new AppInfo(info);

        RB_LOG_RELEASE(LOGTAG_MAIN, "RabBit Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
    }

    Application::~Application()
    {

    }

    bool Application::Start(const char* launch_args)
    {
        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "============== STARTUP ==============");
        RB_LOG(LOGTAG_MAIN, "");

        RB_LOG(LOGTAG_MAIN, "Launch arguments: %s", launch_args)

        char asset_path[256];
        if (const char* offset = std::strstr(launch_args, "-assetPath"); offset != NULL)
        {
            std::string s = offset;

            int start = std::strlen("-assetPath") + 1;
            s = s.substr(start);
            int end = s.find_first_of(" ");
            if (end == std::string::npos)
            {
                end = s.size(); // This is the final argument
            }

            if (s[end-1] != '/' && s[end-1] != '\\')
            {
                s.insert(s.begin() + end, '/');
                end++;
            }

            strcpy(asset_path, s.substr(0, end).c_str());
        }
        else
        {
            RB_ASSERT_ALWAYS_RELEASE(LOGTAG_MAIN, "Did not fill in the asset path! Use the \"-assetPath \"path\" launch argument to specify the path");
            return false;
        }

        RB_LOG(LOGTAG_MAIN, "Asset path: \"%s\"", asset_path);

        AssetManager::Init(asset_path);

#ifdef RB_STEAM_API
        if (std::strstr(launch_args, "-noSteam") == nullptr)
            m_PlatformService = PlatformService::Create(PlatformAPI::Steamworks);
#endif
        if (m_PlatformService && !m_PlatformService->IsInitialized())
            SAFE_DELETE(m_PlatformService);


#if RB_GRAPHICS_API_D3D12
        RenderAPI api = RenderAPI::D3D12;
#elif RB_GRAPHICS_API_VULKAN
        RenderAPI api = RenderAPI::Vulkan;
#else
        RenderAPI api = RenderAPI::None;
#endif

        m_FrameAllocator = new FrameAllocator("Main Allocator", 1, k2MB);

        m_Renderer = Renderer::Create(api, std::strstr(launch_args, "-renderDebug"), std::strstr(launch_args, "-pix"));
        m_Renderer->Init();
        m_Renderer->SetRenderGraphs(m_StartAppInfo->renderGraphs);
        for (auto& [ type, graph ] : m_StartAppInfo->renderGraphs)
        {
            // Remove all memory usage from the graphs as they are now fully built
            graph.Reset();
        }

        m_Displays = Display::CreateDisplays();

        for (const AppInfo::Window& window : m_StartAppInfo->windows)
        {
            if (window.fullscreen && window.windowIndex >= 0)
            {
                RB_ASSERT(LOGTAG_MAIN, window.windowIndex < m_Displays.size(), "Specified window index is invalid");
                int32_t index = Math::Min(window.windowIndex, (int32_t)m_Displays.size());

                m_Windows.push_back(Window::Create(window.windowName, 
                                                   m_Displays[index],
                                                   window.vsync, 
                                                   window.semiTransparent ? kWindowStyle_SemiTransparent : kWindowStyle_Default,
                                                   window.renderScale, window.forcedRenderAspect));
            }
            else
            {
                m_Windows.push_back(Window::Create(window.windowName, 
                                                   window.windowWidth, window.windowHeight, 
                                                   window.vsync, 
                                                   window.semiTransparent ? kWindowStyle_SemiTransparent : kWindowStyle_Default, 
                                                   RenderResourceFormat::BGRA8_UNORM,
                                                   window.renderScale, window.forcedRenderAspect));
            }

            (*(m_Windows.end()-1))->SetVirtualResolutionLinearUpscale(window.linearUpscale);
        }

        m_Scene = new Scene();

        m_Initialized = true;

        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "========== STARTUP COMPLETE =========");
        RB_LOG(LOGTAG_MAIN, "");

        // Initialize app user
        RB_LOG(LOGTAG_MAIN, "Starting user's application: %s", m_StartAppInfo->appName);
        OnStart();

        SAFE_DELETE(m_StartAppInfo);

        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "======== STARTING MAIN LOOP =========");
        RB_LOG(LOGTAG_MAIN, "");

        return true;
    }

    void Application::Run()
    {
        Timer frame_timer;
        double curr_time;
        double prev_time = frame_timer.ElapsedSeconds();

        while (!m_ShouldStop)
        {
            // Update delta time
            curr_time = frame_timer.ElapsedSeconds();
            m_DeltaTime = float(curr_time - prev_time);
            prev_time = curr_time;

            if (m_FixedTimeStep > 0)
            {
                m_DeltaTime = m_FixedTimeStep;
            }

            if (m_PlatformService)
            {
                m_PlatformService->Update();
            }

            // Poll inputs and update windows
            for (Graphics::Window* window : m_Windows)
            {
                if (window->IsValid())
                {
                    window->Update();
                }
                else
                {
                    m_CheckWindows = true;
                }
            }

            // Process the new received events
            ProcessEvents();

            // Firstly update the engine itself
            UpdateInternal(m_DeltaTime);

            // Secondly update the application
            UpdateApp(m_DeltaTime);

            // Submit the scene as context for rendering the next frame
            m_Renderer->SubmitFrame(m_Scene);
            
            // Cycle the allocated scene data for re-use
            m_FrameAllocator->Cycle();

            // Check if there are any windows that should be closed/removed
            if (m_CheckWindows)
            {
                m_Renderer->SyncRenderer(true);

                for (int i = 0; i < m_Windows.size(); i++)
                {
                    if (!m_Windows[i]->IsValid())
                    {
                        delete m_Windows[i];
                        m_Windows.erase(m_Windows.begin() + i);
                        i--;
                    }
                }

                if (m_Windows.size() == 0)
                {
                    RB_LOG(LOGTAG_EVENT, "Last window has been closed, requesting to stop application");
                    m_ShouldStop = true;
                }

                m_CheckWindows = false;
            }

            // Update the frame index
            ++m_FrameIndex;
        }
    }

    void Application::UpdateInternal(float delta_time)
    {

    }

    void Application::UpdateApp(float delta_time)
    {
        // Update the application layers
        for (ApplicationLayer* layer : m_LayerStack)
        {
            if (layer->IsEnabled()) 
            { 
                layer->OnUpdate(delta_time); 
            }
        }

        // Maybe make the scene also just a ApplicationLayer?
        m_Scene->UpdateScene();
    }

    void Application::Shutdown()
    {
        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "============= SHUTDOWN ==============");
        RB_LOG(LOGTAG_MAIN, "");

        for (ApplicationLayer* layer : m_LayerStack)
        {
            layer->OnDetach();
            delete layer;
        }

        m_LayerStack.ClearStack();

        // Shutdown app user
        OnStop();

        delete m_Scene;

        for (int i = 0; i < m_Windows.size(); i++)
        {
            delete m_Windows[i];
        }
        m_Windows.clear();

        for (int i = 0; i < m_Displays.size(); i++)
        {
            delete m_Displays[i];
        }
        m_Displays.clear();

        m_Renderer->Shutdown();
        delete m_Renderer;

        delete m_FrameAllocator;

        SAFE_DELETE(m_PlatformService);

        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "========= SHUTDOWN COMPLETE =========");
        RB_LOG(LOGTAG_MAIN, "");
    }

    void Application::OnNewLayerPushed(ApplicationLayer* layer)
    {
        RB_LOG(LOGTAG_MAIN, "Adding a new layer to the application: %s", layer->GetName());
        layer->OnAttach();
    }

    void Application::PopLayer(ApplicationLayer* layer)
    {
        bool success = m_LayerStack.PopLayer(layer);

        if (success)
        {
            RB_LOG(LOGTAG_MAIN, "Removed layer from the application: %s", layer->GetName());
            layer->OnDetach();
            delete layer;
        }
        else
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Could not find layer %s to remove from the application", layer->GetName());
        }
    }

    Graphics::Window* Application::GetPrimaryWindow() const
    {
        if (m_PrimaryWindowIndex >= m_Windows.size() || m_PrimaryWindowIndex < 0)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "There is no primary window");
            return nullptr;
        }

        return m_Windows[m_PrimaryWindowIndex];
    }

    Graphics::Window* Application::GetWindow(uint32_t index) const
    {
        if (index >= m_Windows.size() || index < 0)
        {
            RB_LOG_WARN(LOGTAG_MAIN, "Trying to get a window that does not exist");
            return nullptr;
        }

        return m_Windows[index];
    }

    Graphics::Window* Application::FindWindow(void* window_handle) const
    {
        for (Graphics::Window* window : m_Windows)
        {
            if (window->IsSameWindow(window_handle))
            {
                return window;
            }
        }

        RB_LOG_WARN(LOGTAG_MAIN, "Could not find the window associated to the window handle");

        return nullptr;
    }

    int32_t Application::FindWindowIndex(void* window_handle) const
    {
        for (int i = 0; i < m_Windows.size(); ++i)
        {
            if (m_Windows[i]->IsSameWindow(window_handle))
            {
                return i;
            }
        }

        RB_LOG_WARN(LOGTAG_MAIN, "Could not find the window associated to the window handle");

        return -1;
    }

    void Application::AddWindow(Graphics::Window* window)
    {
        m_Windows.push_back(window);
    }

    bool Application::OnEvent(Event& event)
    {
        if (!m_Initialized)
        {
            return true;
        }

        // BindEvent<EventType>(RB_BIND_EVENT_FN(Class::Method), event);

        bool passtrough_layers = true;

        BindEvent<KeyPressedEvent>([&](KeyPressedEvent& e)
        {
            if (e.GetKeyCode() == KeyCode::F11 ||
                (IsKeyDown(KeyCode::LeftAlt) && e.GetKeyCode() == KeyCode::Enter))
            {
                passtrough_layers = false;

                WindowFullscreenToggleEvent e(GetPrimaryWindow()->GetNativeWindowHandle());
                g_EventManager->InsertEvent(e);
            }

            if (IsKeyDown(KeyCode::LeftAlt) && e.GetKeyCode() == KeyCode::F4)
            {
                RB_LOG(LOGTAG_EVENT, "Instant close requested, requesting to close all windows..");
                passtrough_layers = false;

                for (int i = 0; i < m_Windows.size(); i++)
                {
                    WindowCloseRequestEvent e(m_Windows[i]->GetNativeWindowHandle());
                    g_EventManager->InsertEvent(e);
                }
            }
        }, event);
        
        BindEvent<WindowOnFocusEvent>([&](WindowOnFocusEvent& focus_event)
        {
            passtrough_layers = false;

            int32_t window_index = FindWindowIndex(focus_event.GetWindowHandle());

            if (window_index >= 0)
            {
                m_PrimaryWindowIndex = window_index;
            }
        }, event);

        BindEvent<WindowCloseRequestEvent>([&](WindowCloseRequestEvent& close_event)
        {
            m_CheckWindows = true;
        }, event);

        if (passtrough_layers)
        {
            // Pass the event to the layers
            for (ApplicationLayer* layer : m_LayerStack)
            {
                if (layer->IsEnabled())
                {
                    // If the event got handled by this layer, do not pass it to any layers after this one
                    if (layer->OnEvent(event))
                    {
                        break;
                    }
                }
            }
        }

        return true;
    }
}