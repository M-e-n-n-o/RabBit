#include <RabBit.h>

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Graphics;
using namespace RB::Math;

#include "EditorWindow.h"
#include "EditorLayer.h"
#include "ImGuiRenderer.h"

class EditorApp : public RB::Application
{
public:
    EditorApp(RB::AppInfo& info) : Application(info) {}

    void OnStart() override
    {
        PushLayer<Editor::EditorLayer>();
    }

    void OnStop() override
    {
    }
};

RB::Application* RB::CreateApplication(const char* launch_args)
{
    AppInfo app_info = {};
    app_info.appName = "RabBit Editor";

    // Using a custom ImGUI window

    app_info.initialRenderGraph = RenderGraphBuilder()
        // Passes
        .AddPass<GBufferPass>           (RenderPassType::GBuffer,           RenderPassSettings{})
        .AddPass<CascadedShadowPass>    (RenderPassType::CascadedShadow,    RenderPassSettings{})
        .AddPass<DeferredLightingPass>  (RenderPassType::DeferredLighting,  RenderPassSettings{})
        .AddPass<Overlay2DPass>         (RenderPassType::Overlay2D,         RenderPassSettings{})
        .AddPass<Editor::ImGuiRenderer> (RenderPassType::Custom0,           RenderPassSettings{})

        // Connections           (from)     ->      (to)
        .AddLink(RenderPassType::GBuffer,           RenderPassType::DeferredLighting, 
                                    0u,                0u,
                                    1u,                1u)

        .AddLink(RenderPassType::CascadedShadow,    RenderPassType::DeferredLighting,
                                    0u,                2u)

        .AddLink(RenderPassType::DeferredLighting,  RenderPassType::Overlay2D,
                                    0u,                0u)

        .AddLink(RenderPassType::Overlay2D,         RenderPassType::Custom0,
                                    0u,                 0u)

        // Finalize
        .SetFinalPass(RenderPassType::Custom0, 0);

    return new EditorApp(app_info);
}