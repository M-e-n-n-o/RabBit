#pragma once

#include "Core.h"
#include "Settings.h"
#include "ApplicationLayer.h"
#include "events/Event.h"

#include <cstdint>

namespace RB
{
    namespace Graphics
    {
        class Window;
        class Display;
        class Renderer;
    }

    namespace Entity
    {
        class Scene;
    }

    struct AppInfo
    {
        struct Window
        {
            const char* windowName          = "RabBit App";
            bool		fullscreen          = false;
            uint32_t    windowWidth         = 1280;
            uint32_t    windowHeight        = 720;
            float		forcedRenderAspect  = 0.0f;
            float		renderScale         = 1.0f;
            float       gammaCorrection     = 2.2f;
            float       brightness          = 1.0f;
            bool		semiTransparent     = false;
        };

        const char*		appName;
        List<Window>	windows;
    };

    class Application : public Events::EventListener
    {
    public:
        Application(AppInfo& info);
        virtual ~Application();

        bool Start(const char* launch_args);
        void Run();
        void Shutdown();

        template<class Layer, typename... Args>
        ApplicationLayer* PushLayer(Args... args);

        template<class Overlay, typename... Args>
        ApplicationLayer* PushOverlay(Args... args);

        void PopLayer(ApplicationLayer* layer);

        List<Graphics::Display*> GetDisplays() const { return m_Displays; }

        Graphics::Window* GetPrimaryWindow() const;
        Graphics::Window* GetWindow(uint32_t index) const;
        Graphics::Window* FindWindow(void* window_handle) const;
        int32_t			  FindWindowIndex(void* window_handle) const;

        void ApplyNewGraphicsSettings(GraphicsSettings& settings);
        GraphicsSettings GetGraphicsSettings() { return m_GraphicsSettings; }
        const GraphicsSettings& GetGraphicsSettings() const { return m_GraphicsSettings; }

        Graphics::Renderer* GetRenderer() const { return m_Renderer; }

        Entity::Scene* GetScene() const { return m_Scene; }

        uint64_t GetFrameIndex() const { return m_FrameIndex; }

        static Application* GetInstance() { return s_Instance; }

    private:
        virtual void OnStart() = 0;
        virtual void OnStop() = 0;

        void UpdateInternal();
        void UpdateApp();
        void OnNewLayerPushed(ApplicationLayer* layer);
        bool OnEvent(Events::Event& event) override;

        const AppInfo				m_StartAppInfo;

        bool						m_Initialized;
        bool						m_ShouldStop;

        uint64_t					m_FrameIndex;

        List<Graphics::Display*>	m_Displays;

        List<Graphics::Window*>		m_Windows;
        int32_t						m_PrimaryWindowIndex;
        bool						m_CheckWindows;

        GraphicsSettings            m_GraphicsSettings;
        Graphics::Renderer*			m_Renderer;

        Entity::Scene*				m_Scene;

        LayerStack                  m_LayerStack;

        static Application*			s_Instance;
    };

    template<class Layer, typename... Args>
    inline ApplicationLayer* Application::PushLayer(Args... args)
    {
        ApplicationLayer* layer = new Layer(args...);
        m_LayerStack.PushLayer(layer);
        OnNewLayerPushed(layer);
        return layer;
    }

    template<class Overlay, typename... Args>
    inline ApplicationLayer* Application::PushOverlay(Args... args)
    {
        ApplicationLayer* overlay = new Overlay(args...);
        m_LayerStack.PushOverlay(overlay);
        OnNewLayerPushed(overlay);
        return overlay;
    }

    // To be defined in client
    Application* CreateApplication(const char* launch_args);
}
