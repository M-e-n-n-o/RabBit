#include <RabBit.h>

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;
using namespace RB::Graphics;

class TestLayer : public ApplicationLayer
{
private:
    Mesh* m_Mesh;
    Material* m_Material;

    GameObject* m_Obj1;
    GameObject* m_Obj2;

    Transform* m_Transform;
    Transform* m_Camera;

    // Degrees
    float m_CameraYaw = 0.0f;
    float m_CameraPitch = 0.0f;

    Font* m_Font;

public:
    TestLayer() : ApplicationLayer("TestLayer") {}

    void OnAttach() override
    {
        RB_LOG("Hoiii");

        float vertex_data[] = {
            // Pos					Color				UV
            -1.0f,  -1.0f, -1.0f,	0.0f, 0.0f, 0.0f,	0, 1,	// 0
            -1.0f,   1.0f, -1.0f,	0.0f, 1.0f, 0.0f,	0, 1,	// 1
             1.0f,   1.0f, -1.0f,	1.0f, 1.0f, 0.0f,	0, 1,	// 2
             1.0f,  -1.0f, -1.0f,	1.0f, 0.0f, 0.0f,	0, 1,	// 3
            -1.0f,  -1.0f,  1.0f,	0.0f, 0.0f, 1.0f,	0, 1,	// 4
            -1.0f,   1.0f,  1.0f,	0.0f, 1.0f, 1.0f,	0, 1,	// 5
             1.0f,   1.0f,  1.0f,	1.0f, 1.0f, 1.0f,	0, 1,	// 6
             1.0f,  -1.0f,  1.0f,	1.0f, 0.0f, 1.0f,	0, 1,	// 7
        };

        uint32_t index_data[] = {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            4, 5, 1, 4, 1, 0,
            3, 2, 6, 3, 6, 7,
            1, 5, 6, 1, 6, 2,
            4, 0, 3, 4, 3, 7
        };

        //float vertex_data[] = {
        //	// Pos				Color
        //	-0.5f, -0.5f, 0,	1, 0, 0,
        //	0, 0.5f, 0,			0, 1, 0,
        //	0.5f, -0.5f, 0,		0, 0, 1,
        //};

        LoadedMesh mesh;
        //bool success = AssetManager::LoadMesh("sponza/NewSponza_Main_Yup_003.fbx", &mesh);
        bool success = AssetManager::LoadMesh("Bunny.fbx", &mesh);

        m_Mesh = new Mesh("Triangle", vertex_data, 8, _countof(vertex_data), index_data, _countof(index_data));
        m_Material = new Material("Test.bc1", true, TextureColorSpace::sRGB);

        Scene* scene = Application::GetInstance()->GetScene();

        List<Material*> materials;
        for (int i = 0; i < mesh.diffuseColorTextures.size(); i++)
        {
            materials.push_back(new Material(("sponza/" + mesh.diffuseColorTextures[i]).c_str()));
        }

        List<MeshRenderer*> meshes;
        for (int i = 0; i < mesh.models.size(); i++)
        {
            if (mesh.models[i].positions.empty())
                continue;

            m_Mesh = new Mesh("Mesh", mesh.models[i]);

            Material* mat = materials[mesh.models[i].diffuseTexIndex];

            GameObject* object = scene->CreateGameObject();
            meshes.push_back(object->AddComponent<MeshRenderer>(m_Mesh, mat));
            Transform* t = object->AddComponent<Transform>();
            t->position = mesh.models[i].position + Float3(0, 0, 300);
            t->scale = Math::Float3(1);

            Float3 r = mesh.models[i].rotation;
            t->rotation = Quaternion::FromEuler(DegreesToRadians(r.x), DegreesToRadians(r.y + 180.0f), DegreesToRadians(r.z));
            
            m_Transform = t;
        }

        void* window_handle0 = Application::GetInstance()->GetWindow(0)->GetNativeWindowHandle();
        //void* window_handle1 = Application::GetInstance()->GetWindow(1)->GetNativeWindowHandle();

        m_Obj1 = scene->CreateGameObject();
        m_Camera = m_Obj1->AddComponent<Transform>();
        m_Obj1->AddComponent<NetworkTransformSync>(true);
        Camera* cam_comp = m_Obj1->AddComponent<Camera>(0.1f, 1000.0f, 70.0f, window_handle0);
        cam_comp->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.4f });


        Mesh* ground = new Mesh("Cube", vertex_data, 8, _countof(vertex_data), index_data, _countof(index_data));

        GameObject* ground_obj = scene->CreateGameObject();
        ground_obj->AddComponent<MeshRenderer>(ground, m_Material);
        auto* ground_t = ground_obj->AddComponent<Transform>();
        ground_obj->AddComponent<NetworkTransformSync>(false);
        //ground_t->position.y = -5;
        ground_t->scale = Float3(5, 5, 5);


        auto* sun = scene->CreateGameObject();
        sun->AddComponent<DirectionalLight>(Math::Float3(-0.3f, -0.98f, 0.0f), Math::Float3(0.99f, 0.97f, 0.76f));

        // UI
        {
            auto canvas = scene->CreateGameObject();
            canvas->AddComponent<UICanvas>(cam_comp);
        
        
            auto list = scene->CreateGameObject();
            auto* list_box = list->AddComponent<UIBox>();
            list_box->SetSize(UIUnit::IPCT, 10, 50);
            list->AddComponent<ListView>(true, 15);
            //auto list_t = list->AddComponent<Transform>();
            //list_t->position.x = 0.0f;
            //list_t->position.y = 0.0f;
            list->SetParent(canvas);
        
            auto rect_obj = scene->CreateGameObject();
            auto* rect_box = rect_obj->AddComponent<UIBox>();
            //rect_box->SetStartPos(UIUnit::PX, 5, 5);
            rect_box->SetPadding(UIUnit::PX, 5);
            //rect_box->AddConstraint(UIConstraintType::Left);
            //rect_box->AddConstraint(UIConstraintType::Right);
            //rect_box->AddConstraint(UIConstraintType::Up);
            //rect_box->AddConstraint(UIConstraintType::Down);
            rect_box->SetSize(UIUnit::IPCT, 25, 25);
            rect_obj->AddComponent<Rect2D>(Math::Float4(0.0f, 1.0f, 0.0f, 0.4f), 0);
            rect_obj->SetParent(list);
        
            LoadedFont font;
            AssetManager::LoadFont("TypoGraphica.otf", &font, 48);
            
            m_Font = new Font("Cool Font", font);
            
            auto text_obj = scene->CreateGameObject();
            auto* text_box = text_obj->AddComponent<UIBox>();
            text_box->SetSize(UIUnit::IPCT, 30, 10);
            text_obj->AddComponent<Text2D>(m_Font, "Hoi Sylvia! (dit zie je niet)", 1.0f, 0);
            text_obj->SetParent(list);
        }

        //m_Obj2 = scene->CreateGameObject();
        //m_Obj2->AddComponent<Transform>();
        //Camera* cam_comp2 = m_Obj2->AddComponent<Camera>(0.01f, 1000.0f, 90.0f, window_handle1);
        //cam_comp2->SetClearColor({ 1.0f, 0.3f, 0.3f, 0.0f });
    }

    void OnUpdate(float delta) override
    {
        if (Input::IsKeyDown(KeyCode::Q))
            m_Transform->rotation = m_Transform->rotation * Quaternion::FromAxisAngle(WorldUp, DegreesToRadians(25.0f * delta));
        if (Input::IsKeyDown(KeyCode::E))
            m_Transform->rotation = m_Transform->rotation * Quaternion::FromAxisAngle(WorldRight, DegreesToRadians(25.0f * delta));
        m_Transform->rotation.Normalize();

        static Float2 last_pos = Input::GetMousePos();
        Float2 new_pos = Input::GetMousePos();

        if (Input::IsMouseKeyDown(MouseCode::ButtonRight))
        {
            Float2 vel = (new_pos - last_pos) * 0.2f;
            m_CameraPitch += vel.y;
            m_CameraYaw   += vel.x;
        }

        last_pos = new_pos;

        m_CameraPitch = Clamp(m_CameraPitch, -89.0f, 89.0f);

        m_Camera->rotation = Quaternion::FromEuler(DegreesToRadians(m_CameraPitch), DegreesToRadians(m_CameraYaw), 0.0f);

        Float3 forward = m_Camera->rotation.Rotate(WorldForward);
        Float3 right   = m_Camera->rotation.Rotate(WorldRight);
        Float3 up      = m_Camera->rotation.Rotate(WorldUp);

        // Move forward/backward
        if (Input::IsKeyDown(KeyCode::W))
            m_Camera->position = m_Camera->position + (forward * (50 * delta));
        if (Input::IsKeyDown(KeyCode::S))
            m_Camera->position = m_Camera->position - (forward * (50 * delta));

        // Strafe left/right
        if (Input::IsKeyDown(KeyCode::A))
            m_Camera->position = m_Camera->position - (right * (50 * delta));
        if (Input::IsKeyDown(KeyCode::D))
            m_Camera->position = m_Camera->position + (right * (50 * delta));

        // Move up/down
        if (Input::IsKeyDown(KeyCode::Space))
            m_Camera->position = m_Camera->position + (up * (50 * delta));
        if (Input::IsKeyDown(KeyCode::LeftShift))
            m_Camera->position = m_Camera->position - (up * (50 * delta));
    }

    bool OnEvent(Event& event) override
    {
        if (event.GetEventType() == EventType::KeyPressed)
        {
            const KeyPressedEvent& pressed_event = (const KeyPressedEvent&)event;

            auto* network_service = Application::GetInstance()->GetNetworkService();

            if (network_service && pressed_event.GetKeyCode() == KeyCode::C)
            {
                if (network_service->IsConnected())
                    network_service->LeaveLobby();
                else
                    network_service->CreateLobby(LobbyType::FriendsOnly, 8);
            }
            if (network_service && pressed_event.GetKeyCode() == KeyCode::J)
            {
                if (network_service->IsConnected())
                    network_service->LeaveLobby();
                else
                {
                    const char ip[] = "127.0.0.1";
                    network_service->JoinLobby((uint64_t)&ip);
                }
            }

            if (network_service && network_service->IsConnected() && pressed_event.GetKeyCode() == KeyCode::I)
            {
                network_service->OpenInviteFriendsOverlay();
            }
        }

        if (event.GetEventType() == EventType::WindowCloseRequest)
        {
            const WindowCloseRequestEvent& close_event = (const WindowCloseRequestEvent&)event;

            Scene* scene = Application::GetInstance()->GetScene();
            int32_t window_index = Application::GetInstance()->FindWindowIndex(close_event.GetWindowHandle());

            if (window_index == 0)
                scene->RemoveGameObject(m_Obj1);
            else if (window_index == 1)
                scene->RemoveGameObject(m_Obj2);

            return true;
        }

        return false;
    }

    void OnDetach() override
    {
        delete m_Mesh;
        delete m_Material;
        delete m_Font;
    }
};