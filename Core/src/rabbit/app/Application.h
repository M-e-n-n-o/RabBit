#pragma once

#include "Core.h"
#include "ApplicationLayer.h"
#include "FrameAllocator.h"
#include "events/Event.h"
#include "graphics/Renderer.h"

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
            const char* windowName             = "RabBit App";
            bool        fullscreen             = false;
            int32_t     windowIndex            = -1;
            uint32_t    windowWidth            = 1280;
            uint32_t    windowHeight           = 720;
            bool        vsync                  = true;
            float       forcedRenderAspect     = 0.0f;
            float       renderScale            = 1.0f;
            bool        linearUpscale          = true;
            bool        semiTransparent        = false;
        };

        const char*                     appName;
        List<Window>                    windows;

        UnorderedMap<Graphics::RenderGraphType, Graphics::RenderGraphBuilder> renderGraphs;
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
        Layer* PushLayer(Args... args);

        template<class Overlay, typename... Args>
        Overlay* PushOverlay(Args... args);

        void PopLayer(ApplicationLayer* layer);

        List<Graphics::Display*> GetDisplays() const { return m_Displays; }

        Graphics::Window* GetPrimaryWindow() const;
        Graphics::Window* GetWindow(uint32_t index) const;
        Graphics::Window* FindWindow(void* window_handle) const;
        int32_t           FindWindowIndex(void* window_handle) const;
        void              AddWindow(Graphics::Window* window);

        Graphics::Renderer* GetRenderer() const { return m_Renderer; }

        Entity::Scene* GetScene() const { return m_Scene; }

        uint64_t GetFrameIndex() const { return m_FrameIndex; }
        float GetDeltaTime() const { return m_DeltaTime; }

        void EnableFixedTimeStep(float time_step) { m_FixedTimeStep = time_step; }
        void DisableFixedTimeStep() { m_FixedTimeStep = -1; }

        FrameAllocator* GetAllocator() const { return m_FrameAllocator; }

        static Application* GetInstance() { return s_Instance; }

    private:
        virtual void OnStart() = 0;
        virtual void OnStop() = 0;

        void UpdateInternal(float delta_time);
        void UpdateApp(float delta_time);
        void OnNewLayerPushed(ApplicationLayer* layer);
        bool OnEvent(Events::Event& event) override;

        AppInfo*                    m_StartAppInfo;

        bool                        m_Initialized;
        bool                        m_ShouldStop;

        List<Graphics::Display*>    m_Displays;

        List<Graphics::Window*>     m_Windows;
        int32_t                     m_PrimaryWindowIndex;
        bool                        m_CheckWindows;

        Graphics::Renderer*         m_Renderer;

        Entity::Scene*              m_Scene;

        FrameAllocator*             m_FrameAllocator;

        LayerStack                  m_LayerStack;

        uint64_t                    m_FrameIndex;
        float                       m_DeltaTime;
        float                       m_FixedTimeStep;

        static Application*         s_Instance;
    };

    template<class Layer, typename... Args>
    inline Layer* Application::PushLayer(Args... args)
    {
        Layer* layer = new Layer(args...);
        m_LayerStack.PushLayer(layer);
        OnNewLayerPushed(layer);
        return layer;
    }

    template<class Overlay, typename... Args>
    inline Overlay* Application::PushOverlay(Args... args)
    {
        Overlay* overlay = new Overlay(args...);
        m_LayerStack.PushOverlay(overlay);
        OnNewLayerPushed(overlay);
        return overlay;
    }

    // To be defined in client
    Application* CreateApplication(const char* launch_args);
}
