#pragma once

#include <RabBit.h>

#include "EditorWindow.h"
#include "ImGuiManager.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
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
        EditorWindow* m_Window;
        ImGuiContext* m_ImGuiRenderContext;

    public:
        EditorLayer() : ApplicationLayer("EditorLayer") 
        {
            InitializeImGui();

            m_Window = new Editor::EditorWindow("RabBit Editor");
            Application::GetInstance()->AddWindow(m_Window);

            m_ImGuiRenderContext = CreateImGuiContext();
            InitializeImGuiContextRenderBackend(m_ImGuiRenderContext, m_Window->GetBackBufferFormat());
        }

        void OnAttach() override
        {
            Scene* scene = Application::GetInstance()->GetScene();

            auto* obj = scene->CreateGameObject();
            obj->AddComponent<ImGuiManager>(m_ImGuiRenderContext);
            obj->AddComponent<Transform>();
            Camera* cam_comp = obj->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, m_Window->GetNativeWindowHandle());
            cam_comp->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.4f });
        }

        bool show = true;
        void OnUpdate(float delta) override
        {
            m_Window->PrepareDraw();
            ImGui::Begin("Test window", &show);
            ImGui::Text("Hello World");
            ImGui::End();
            ImGui::ShowDemoWindow(&show);
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