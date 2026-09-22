#pragma once

#include "Mp4Encoder.h"
#include "ImGuiManager.h"

#include "panels/ViewportPanel.h"
#include "engine/EngineEditorWindow.h"

#include <RabBit.h>

namespace Editor
{
    class ViewerLayer : public RB::ApplicationLayer
    {
    public:
        ViewerLayer(const char* model_name);

        void OnAttach() override;

        void OnUpdate(float delta) override;

        bool OnEvent(RB::Events::Event& event) override;

        void OnDetach() override;

    private:
        ImGuiContext*                           m_ImGuiRenderContext;
        EngineEditorWindow*                     m_Window;

        ViewportPanel*                          m_Viewport;

        RB::Entity::Camera*                     m_Camera;
        RB::Entity::Transform*                  m_CamTransform;
        RB::Entity::SceneUtils::SpawnedModel    m_Model;

        float                                   m_OrbitYaw = 0.0f;
        float                                   m_OrbitPitch = 0.0f;
        float                                   m_OrbitDistance = 10.0f;
        float                                   m_MinOrbitDistance = 0.1f;
        float                                   m_MaxOrbitDistance = 5000.0f;

        const char*                             m_ModelName;
    };
}