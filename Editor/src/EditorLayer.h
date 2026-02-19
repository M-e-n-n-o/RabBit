#pragma once

// We need these defines to be able to access the platform files inside RabBit
#define RB_GRAPHICS_API_D3D12 1
#define RB_PLATFORM_WINDOWS 1
#include <RabBit.h>

#include "engine/EngineEditorWindow.h"
#include "ImGuiManager.h"

#include "platform/graphics/d3d12/resource/RenderResourceD3D12.h"
#include "platform/graphics/d3d12/resource/Descriptor.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;
using namespace RB::Graphics;

namespace Editor
{
    class EditorLayer : public ApplicationLayer
    {
    private:
        EngineEditorWindow* m_Window;
        EngineEditorWindow* m_Window1;
        ImGuiContext* m_ImGuiRenderContext;
        Shared<Texture2D> m_SceneTexture;

    public:
        EditorLayer() : ApplicationLayer("EditorLayer") 
        {
            InitializeImGui();

            m_Window = new Editor::EngineEditorWindow("RabBit Editor");
            Application::GetInstance()->AddWindow(m_Window);
            //m_Window1 = new Editor::EngineEditorWindow("RabBit Editor2");
            //Application::GetInstance()->AddWindow(m_Window1);

            m_ImGuiRenderContext = CreateImGuiContext();
            InitializeImGuiContextRenderBackend(m_ImGuiRenderContext, m_Window->GetBackBufferFormat());
        }

        void OnAttach() override
        {
            Scene* scene = Application::GetInstance()->GetScene();

            auto* context_obj = scene->CreateGameObject();
            context_obj->AddComponent<ImGuiManager>(m_ImGuiRenderContext);

            m_SceneTexture = Texture2D::Create("Game scene", RenderResourceFormat::R8G8B8A8_UNORM, 1280, 720, true, true);

            // This camera renders the viewport
            auto* game_cam_obj = scene->CreateGameObject();
            game_cam_obj->AddComponent<Transform>();
            Camera* cam_comp = game_cam_obj->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, m_SceneTexture, kRenderGraphType_Normal);
            cam_comp->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.4f });

            // This camera just renders the ImGui stuff on the OS window
            auto* imgui_cam = scene->CreateGameObject();
            imgui_cam->AddComponent<Transform>();
            imgui_cam->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, m_Window->GetNativeWindowHandle(), kRenderGraphType_Post);
        }

        void OnUpdate(float delta) override
        {
            m_Window->SelectForDraw();
            ImGui::Begin("Test window");
            ImGui::Text("Hello World");
            ImGui::Image((ImTextureID)(D3D12::g_DescriptorManager->GetGpuHandle((std::static_pointer_cast<D3D12::Texture2DD3D12>(m_SceneTexture)->GetSrvHandle())).ptr), ImVec2(1280, 720));
            ImGui::End();
            m_Window->DeselectForDraw();

            //m_Window1->SelectForDraw();
            //ImGui::Begin("Test window");
            //ImGui::Text("Hello World");
            //ImGui::End();
            //m_Window1->DeselectForDraw();
        }

        bool OnEvent(const Event& event) override
        {
            return false;
        }

        void OnDetach() override
        {
            DestroyImGuiContext(m_ImGuiRenderContext);
        }
    };
}