#pragma once

#include <rabbit/app/ApplicationLayer.h>

#include "panels/ViewportPanel.h"
#include "panels/ConsolePanel.h"
#include "engine/EngineEditorWindow.h"
#include "ImGuiManager.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"

namespace Editor
{
    class EditorLayer : public RB::ApplicationLayer
    {
    public:
        EditorLayer();

        void OnAttach() override;

        void OnUpdate(float delta) override;

        bool OnEvent(const RB::Events::Event& event) override;

        void OnDetach() override;

        bool CustomLogging(int mode, const char* text);

    private:
        ImGuiContext* m_ImGuiRenderContext;
        EngineEditorWindow* m_Window;

        ViewportPanel* m_Viewport;
        ConsolePanel* m_Console;

        RB::Entity::Camera* m_Camera;
        RB::Entity::Mesh* m_TriangleMesh;
        RB::Entity::Material* m_Material;
    };
}