#include "ViewerLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"

#include <RabBit.h>

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;
using namespace RB::Graphics;

namespace Editor
{
    ViewerLayer::ViewerLayer(const char* model_name)
        : ApplicationLayer("ViewerLayer")
        , m_ModelName(model_name)
        , m_Camera(nullptr)
    {
        InitializeImGui(false);

        m_Window = new Editor::EngineEditorWindow("RabBit Editor");
        Application::GetInstance()->AddWindow(m_Window);

        m_ImGuiRenderContext = CreateImGuiContext();
        InitializeImGuiContextRenderBackend(m_ImGuiRenderContext, m_Window->GetBackBufferFormat());

        m_Viewport = m_Window->AddPanel<ViewportPanel>(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    }

    void ViewerLayer::OnAttach()
    {
        Scene* scene = Application::GetInstance()->GetScene();

        auto* context_obj = scene->CreateGameObject();
        context_obj->AddComponent<ImGuiManager>(m_ImGuiRenderContext);

        // Scene
        {
            LoadedModel model;
            bool success = AssetLoader::LoadConvertedModel(m_ModelName, &model);
            if (!success)
            {
                return;
            }

            List<Material*> materials;
            for (int i = 0; i < model.diffuseColorTextures.size(); i++)
            {
                materials.push_back(new Material(model.diffuseColorTextures[i].c_str(), false));
            }

            m_Model = SceneUtils::SpawnModel(scene, &model, materials);

            // This camera renders the viewport
            auto* game_cam_obj = scene->CreateGameObject();
            m_CamTransform = game_cam_obj->AddComponent<Transform>();
            m_Camera = game_cam_obj->AddComponent<Camera>(0.1f, 10000.0f, 70.0f, m_Viewport->GetSceneTexture(), kRenderGraphType_Normal);
            m_Camera->SetClearColor({ 0.3f, 0.3f, 0.3f, 1.0f });

            Math::Float3 model_bounds(0);
            for (const auto& submodel : model.models)
            {
                model_bounds.x = Math::Max(model_bounds.x, submodel.maxBounds.x);
                model_bounds.y = Math::Max(model_bounds.y, submodel.maxBounds.y);
                model_bounds.z = Math::Max(model_bounds.z, submodel.maxBounds.z);
            }
            float cam_distance = Math::Max(model_bounds.x, Math::Max(model_bounds.y, model_bounds.z)) * 4.0f;
            m_OrbitDistance = cam_distance;
            m_OrbitYaw      = 0.0f;
            m_OrbitPitch    = 0.0f;
            m_CamTransform->position.z = -cam_distance;

            auto* sun = scene->CreateGameObject("Sun");
            sun->AddComponent<DirectionalLight>(Math::Float3(-0.3f, -0.98f, 0.0f), Math::Float3(0.99f, 0.97f, 0.76f));
        }

        // This camera just renders the ImGui stuff on the OS window
        auto* imgui_cam = scene->CreateGameObject();
        imgui_cam->AddComponent<Transform>();
        imgui_cam->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, m_Window->GetNativeWindowHandle(), kRenderGraphType_Post);
    }

    void ViewerLayer::OnUpdate(float delta)
    {
        if (m_Camera == nullptr)
        {
            return;
        }

        m_Camera->SetRenderTexture(m_Viewport->GetSceneTexture());

        // Rotating
        if (Input::IsMouseKeyDown(MouseCode::ButtonRight))
        {
            Math::Float2 mouse_delta = Input::GetMousePosDelta();

            const float rotate_speed = 0.005f;
            m_OrbitYaw -= mouse_delta.x * rotate_speed;
            m_OrbitPitch += mouse_delta.y * rotate_speed;

            // Clamp pitch to avoid flipping over the poles
            const float pitch_limit = Math::DegreesToRadians(89.0f);
            m_OrbitPitch = Math::Clamp(m_OrbitPitch, -pitch_limit, pitch_limit);
        }

        // Zooming
        float scroll_delta = Input::GetMouseScrollDelta();
        const float zoom_speed = m_OrbitDistance * 0.1f;
        m_OrbitDistance -= scroll_delta * zoom_speed;
        m_OrbitDistance = Math::Clamp(m_OrbitDistance, m_MinOrbitDistance, m_MaxOrbitDistance);

        float cos_pitch = Math::Cos(m_OrbitPitch);
        Math::Float3 offset;
        offset.x = m_OrbitDistance * cos_pitch * Math::Sin(m_OrbitYaw);
        offset.y = m_OrbitDistance * Math::Sin(m_OrbitPitch);
        offset.z = -m_OrbitDistance * cos_pitch * Math::Cos(m_OrbitYaw);

        m_CamTransform->position = offset;

        Math::Float3 forward = m_CamTransform->position * -1;
        forward.Normalize();

        m_CamTransform->rotation = Math::Quaternion::LookRotation(forward, Math::Float3(0.0f, 1.0f, 0.0f));
    }

    bool ViewerLayer::OnEvent(Event& event)
    {
        return false;
    }

    void ViewerLayer::OnDetach()
    {
        for (Mesh* mesh : m_Model.meshes)
            delete mesh;
        for (Animation* anim : m_Model.animations)
            delete anim;

        DestroyImGuiContext(m_ImGuiRenderContext);
    }
}