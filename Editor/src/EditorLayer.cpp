#include "EditorLayer.h"

#include <RabBit.h>

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;
using namespace RB::Graphics;

namespace Editor
{
    class TestWindowPanel : public WindowPanel
    {
    public:
        int currentID = 0;

        void OnCreate()
        {
            static int id = 0;
            currentID = id;
            id++;
        }

        void OnDestroy()
        {

        }

        void OnUpdate()
        {
            ImGui::Begin(std::to_string(currentID).c_str());
            ImGui::Text("Hello World");
            ImGui::End();
        }
    };

    EditorLayer::EditorLayer() 
        : ApplicationLayer("EditorLayer")
    {
        InitializeImGui();

        m_Window = new Editor::EngineEditorWindow("RabBit Editor");
        Application::GetInstance()->AddWindow(m_Window);

        m_ImGuiRenderContext = CreateImGuiContext();
        InitializeImGuiContextRenderBackend(m_ImGuiRenderContext, m_Window->GetBackBufferFormat());

        m_Viewport = m_Window->AddPanel<ViewportPanel>();
        m_Console = m_Window->AddPanel<ConsolePanel>();
        m_Hierarchy = m_Window->AddPanel<HierarchyPanel>();
        m_Window->AddPanel<TestWindowPanel>();

        m_Recording = false;
        m_Mp4Encoder = nullptr;
        m_ScreenCaptureData = nullptr;
    }

    void EditorLayer::OnAttach()
    {
        Scene* scene = Application::GetInstance()->GetScene();

        auto* context_obj = scene->CreateGameObject();
        context_obj->AddComponent<ImGuiManager>(m_ImGuiRenderContext);

        // Scene
        {
            // This camera renders the viewport
            auto* game_cam_obj = scene->CreateGameObject();
            game_cam_obj->AddComponent<Transform>();
            m_Camera = game_cam_obj->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, m_Viewport->GetSceneTexture(), kRenderGraphType_Normal);
            m_Camera->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.4f });

            m_ScreenCapturer = game_cam_obj->AddComponent<ScreenCapturer>();

            float vertex_data[] = {
                // Pos              Normal      UV
                -0.5f, -0.5f, 0,    0, 1, 0,    0, 1,
                 0,     0.5f, 0,    0, 1, 0,    0, 1,
                 0.5f, -0.5f, 0,    0, 1, 0,    0, 1
            };
            m_TriangleMesh = new Mesh("Triangle", vertex_data, 8, _countof(vertex_data));
            m_Material = new Material();

            GameObject* triangle_obj = scene->CreateGameObject("Triangle");
            triangle_obj->AddComponent<MeshRenderer>(m_TriangleMesh, m_Material);
            auto* t = triangle_obj->AddComponent<Transform>();
            t->position.z = 5;

            auto* sun = scene->CreateGameObject("Sun");
            sun->AddComponent<DirectionalLight>(Math::Float3(-0.3f, -0.98f, 0.0f), Math::Float3(0.99f, 0.97f, 0.76f));
            sun->SetParent(triangle_obj);
        }

        // This camera just renders the ImGui stuff on the OS window
        auto* imgui_cam = scene->CreateGameObject();
        imgui_cam->AddComponent<Transform>();
        imgui_cam->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, m_Window->GetNativeWindowHandle(), kRenderGraphType_Post);

        m_Hierarchy->SetRoot(scene);
    }

    void EditorLayer::OnUpdate(float delta)
    {
        // Make sure to update the output texture for if it got updated
        m_Camera->SetRenderTexture(m_Viewport->GetSceneTexture());

        if (m_Recording)
        {
            if (m_ScreenCapturer->ReadCapture())
            {
                m_Mp4Encoder->AddFrame(m_ScreenCaptureData);
            }

            m_ScreenCapturer->Capture(m_ScreenCaptureData);
        }
    }

    bool EditorLayer::OnEvent(const Event& event)
    {
        if (event.GetEventType() == EventType::KeyPressed)
        {
            if (((const KeyPressedEvent&)event).GetKeyCode() == KeyCode::P)
            {
                if (m_Recording)
                {
                    m_Mp4Encoder->Finish();
                    SAFE_DELETE(m_Mp4Encoder);
                    SAFE_FREE(m_ScreenCaptureData);
                    m_Recording = false;

                    Application::GetInstance()->DisableFixedTimeStep();
                }
                else
                {
                    const float fps = 60.0f;

                    Shared<Texture2D> tex = m_Viewport->GetSceneTexture();
                    m_Mp4Encoder = new Mp4Encoder("RabBitRender.mp4", tex->GetWidth(), tex->GetHeight(), fps, 4000000, tex->GetFormat());

                    if (m_Mp4Encoder->IsValid())
                    {
                        m_Recording = true;

                        uint64_t size = m_ScreenCapturer->PrepareCapture(m_Viewport->GetSceneTexture().get());
                        m_ScreenCaptureData = (uint8_t*)ALLOC_HEAP(size);

                        Application::GetInstance()->EnableFixedTimeStep(1.0f / fps);
                    }
                    else
                    {
                        SAFE_DELETE(m_Mp4Encoder);
                    }
                }
            }
        }

        return false;
    }

    void EditorLayer::OnDetach()
    {
        delete m_TriangleMesh;
        delete m_Material;
        DestroyImGuiContext(m_ImGuiRenderContext);
    }

    bool EditorLayer::CustomLogging(int mode, const char* text)
    {
        if (m_Console)
        {
            m_Console->AppendLog(mode, text);
            return true;
        }

        return false;
    }
}