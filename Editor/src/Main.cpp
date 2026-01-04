// We need these defines to be able to access the platform files inside RabBit
#define RB_GRAPHICS_API_D3D12 1
#define RB_PLATFORM_WINDOWS 1
#include <RabBit.h>

using namespace RB;
using namespace RB::Graphics;

#include "EditorLayer.h"
#include "ImGuiRenderPass.h"

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

    app_info.renderGraphs = {
            {
                kRenderGraphType_Normal,
                RenderGraphBuilder()
                // Passes
                .AddPass<GBufferPass>           (RenderPassType::GBuffer,           RenderPassSettings{})
                .AddPass<CascadedShadowPass>    (RenderPassType::CascadedShadow,    RenderPassSettings{})
                .AddPass<DeferredLightingPass>  (RenderPassType::DeferredLighting,  RenderPassSettings{})
                .AddPass<Overlay2DPass>         (RenderPassType::Overlay2D,         RenderPassSettings{})

                // Connections           (from)     ->      (to)
                .AddLink(RenderPassType::GBuffer,           RenderPassType::DeferredLighting, 
                                            0u,                0u,
                                            1u,                1u)

                .AddLink(RenderPassType::CascadedShadow,    RenderPassType::DeferredLighting,
                                            0u,                2u)

                .AddLink(RenderPassType::DeferredLighting,  RenderPassType::Overlay2D,
                                            0u,                0u)

                // Finalize
                .SetFinalPass(RenderPassType::Overlay2D, 0)
            },
            {
                kRenderGraphType_Post,
                RenderGraphBuilder()
                .AddPass<Editor::ImGuiRenderPass>(RenderPassType::Custom0, RenderPassSettings{})
                .SetFinalPass(RenderPassType::Custom0, 0)
            },
        };

    return new EditorApp(app_info);
}