#define RB_DEFINE_ENTRY_POINT
#include <RabBit.h>
#include "TestLayer.h"
#include "RayMarcherPass.h"

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;

class SampleApp : public RB::Application
{
public:
    SampleApp(RB::AppInfo& info) : Application(info) {}

    void OnStart() override
    {
        PushLayer<TestLayer>();
    }

    void OnStop() override
    {
    }
};

RB::Application* RB::CreateApplication(const char* launch_args)
{
    AppInfo app_info = {};
    app_info.appName            = "RabBit App";

    AppInfo::Window window1 = {};
    window1.windowName          = "Window 1";
    window1.fullscreen          = false;
    window1.windowIndex         = 0;
    //window1.vsync               = false;
    //window1.windowWidth         = 1280;
    //window1.windowHeight        = 720;
    //window1.forcedRenderAspect  = 4.0f / 3.0f;
    window1.semiTransparent     = true;
    //window1.renderScale         = 0.75f;
    app_info.windows.push_back(window1);

    AppInfo::Window window2 = {};
    window2.windowName          = "Window 2";
    window2.fullscreen          = true;
    //window2.semiTransparent     = true;
    window2.windowWidth         = 1280;
    window2.windowHeight        = 720;
    window2.forcedRenderAspect  = 0.0f;
    window2.renderScale         = 0.25f;
    //app_info.windows.push_back(window2);

    app_info.renderGraphs = { 
        {   
            kRenderGraphType_Normal,
            RenderGraphBuilder()
            // Passes
            .AddPass<GBufferPass>           (RenderPassType::GBuffer,           RenderPassSettings{})
            .AddPass<RayMarcherPass>        (RenderPassType::Custom0,           RayMarcherSettings{})
            .AddPass<CascadedShadowPass>    (RenderPassType::CascadedShadow,    RenderPassSettings{})
            .AddPass<DeferredLightingPass>  (RenderPassType::DeferredLighting,  RenderPassSettings{})
            .AddPass<Overlay2DPass>         (RenderPassType::Overlay2D,         RenderPassSettings{})
            .AddPass<SmaaPass>              (RenderPassType::Smaa,              RenderPassSettings{})
            .AddPass<ToneMappingPass>       (RenderPassType::ToneMapping,       ToneMappingSettings{})

            // Connections           (from)     ->      (to)
            .AddLink(RenderPassType::GBuffer,           RenderPassType::Custom0,
                                        0u,                0u,
                                        1u,                1u)

            .AddLink(RenderPassType::Custom0,           RenderPassType::DeferredLighting,
                                        0u,                0u,
                                        1u,                1u)

            .AddLink(RenderPassType::CascadedShadow,    RenderPassType::DeferredLighting,
                                        0u,                2u)

            .AddLink(RenderPassType::DeferredLighting,  RenderPassType::Overlay2D,
                                        0u,                0u)

            .AddLink(RenderPassType::Overlay2D,         RenderPassType::Smaa,
                                        0u,                0u)

            .AddLink(RenderPassType::Smaa,              RenderPassType::ToneMapping,
                                        0u,                0u)

            // Finalize
            .SetFinalPass(RenderPassType::ToneMapping, 0)
        }
    };

    return new SampleApp(app_info);
}