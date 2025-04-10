#include "RabBitCommon.h"
#include "Application.h"
#include "AssetManager.h"

#include "graphics/Window.h"
#include "graphics/RenderInterface.h"
#include "graphics/Renderer.h"
#include "graphics/Display.h"

#include "entity/Scene.h"

#include "events/ApplicationEvent.h"
#include "events/KeyEvent.h"
#include "events/input/KeyCodes.h"
#include "events/input/Input.h"

using namespace RB::Graphics;
using namespace RB::Events;
using namespace RB::Entity;

namespace RB
{
    Application* Application::s_Instance = nullptr;

    Application::Application(AppInfo& info)
        : EventListener(kEventCat_All)
        , m_StartAppInfo(info)
        , m_Initialized(false)
        , m_ShouldStop(false)
        , m_FrameIndex(0)
        , m_CheckWindows(false)
        , m_PrimaryWindowIndex(0)
    {
        RB_ASSERT_FATAL(LOGTAG_MAIN, s_Instance == nullptr, "Application already exists");
        s_Instance = this;

        RB_LOG_RELEASE(LOGTAG_MAIN, "Welcome to the RabBit Engine");
        RB_LOG_RELEASE(LOGTAG_MAIN, "Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
    }

    Application::~Application()
    {

    }

    bool Application::Start(const char* launch_args)
    {
        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "============== STARTUP ==============");
        RB_LOG(LOGTAG_MAIN, "");

        char asset_path[256];
        if (const char* offset = std::strstr(launch_args, "-assetPath"); offset != NULL)
        {
            std::string s = offset;

            int start = s.find_first_of("\"") + 1;
            s = s.substr(start);
            int end = s.find_first_of("\"");

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

        Renderer::SetAPI(RenderAPI::D3D12);

        m_GraphicsSettings = {};
        //m_GraphicsSettings.renderWidth = // What size to set here??

        m_GraphicsSettings.Print();

        m_Renderer = Renderer::Create(std::strstr(launch_args, "-renderDebug"));
        m_Renderer->Init();

        m_Displays = Display::CreateDisplays();

        for (const AppInfo::Window& window : m_StartAppInfo.windows)
        {
            if (window.fullscreen)
            {
                m_Windows.push_back(Window::Create(window.windowName, m_Displays[0], window.semiTransparent ? kWindowStyle_SemiTransparent : kWindowStyle_Default, window.renderScale, window.forcedRenderAspect));
            }
            else
            {
                m_Windows.push_back(Window::Create(window.windowName, window.windowWidth, window.windowHeight, window.semiTransparent ? kWindowStyle_SemiTransparent : kWindowStyle_Default, RenderResourceFormat::R8G8B8A8_UNORM, window.renderScale, window.forcedRenderAspect));
            }

            (*(m_Windows.end()-1))->SetBrightness(window.brightness);
            (*(m_Windows.end()-1))->SetGammaCorrection(window.gammaCorrection);
        }

        m_Scene = new Scene();

        m_Initialized = true;

        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "========== STARTUP COMPLETE =========");
        RB_LOG(LOGTAG_MAIN, "");

        // Initialize app user
        RB_LOG(LOGTAG_MAIN, "Starting user's application: %s", m_StartAppInfo.appName);
        OnStart();

        RB_LOG(LOGTAG_MAIN, "");
        RB_LOG(LOGTAG_MAIN, "======== STARTING MAIN LOOP =========");
        RB_LOG(LOGTAG_MAIN, "");

        return true;
    }

    void Application::Run()
    {
        while (!m_ShouldStop)
        {
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
            UpdateInternal();

            // Secondly update the application
            UpdateApp();

            // Submit the scene as context for rendering the next frame
            m_Renderer->SubmitFrame(m_Scene);

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

    void Application::UpdateInternal()
    {

    }

    void Application::UpdateApp()
    {
        // Update the application layers
        for (ApplicationLayer* layer : m_LayerStack)
        {
            if (layer->IsEnabled()) 
            { 
                layer->OnUpdate(); 
            }
        }
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

    void Application::ApplyNewGraphicsSettings(GraphicsSettings& settings)
    {
        settings.Validate();
        settings.Print();

        GraphicsSettingsChangedEvent e(settings, m_GraphicsSettings);
        g_EventManager->InsertEvent(e);
    }

    void Application::OnEvent(Event& event)
    {
        if (!m_Initialized)
        {
            return;
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

        
        BindEvent<GraphicsSettingsChangedEvent>([&](GraphicsSettingsChangedEvent& settings_event)
        {
            m_GraphicsSettings = settings_event.GetNewSettings();
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
    }
}