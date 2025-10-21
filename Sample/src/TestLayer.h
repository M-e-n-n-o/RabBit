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

    Font* m_Font;

public:
    TestLayer() : ApplicationLayer("TestLayer") {}

    void OnAttach() override
    {
        RB_LOG("Hoiii");

        //float vertex_data[] = {
        //    // Pos					Color				UV
        //    -1.0f,  -1.0f, -1.0f,	0.0f, 0.0f, 0.0f,	0, 1,	// 0
        //    -1.0f,   1.0f, -1.0f,	0.0f, 1.0f, 0.0f,	0, 1,	// 1
        //     1.0f,   1.0f, -1.0f,	1.0f, 1.0f, 0.0f,	0, 1,	// 2
        //     1.0f,  -1.0f, -1.0f,	1.0f, 0.0f, 0.0f,	0, 1,	// 3
        //    -1.0f,  -1.0f,  1.0f,	0.0f, 0.0f, 1.0f,	0, 1,	// 4
        //    -1.0f,   1.0f,  1.0f,	0.0f, 1.0f, 1.0f,	0, 1,	// 5
        //     1.0f,   1.0f,  1.0f,	1.0f, 1.0f, 1.0f,	0, 1,	// 6
        //     1.0f,  -1.0f,  1.0f,	1.0f, 0.0f, 1.0f,	0, 1,	// 7
        //};

        //uint32_t index_data[] = {
        //    0, 1, 2, 0, 2, 3,
        //    4, 6, 5, 4, 7, 6,
        //    4, 5, 1, 4, 1, 0,
        //    3, 2, 6, 3, 6, 7,
        //    1, 5, 6, 1, 6, 2,
        //    4, 0, 3, 4, 3, 7
        //};

        //float vertex_data[] = {
        //	// Pos				Color
        //	-0.5f, -0.5f, 0,	1, 0, 0,
        //	0, 0.5f, 0,			0, 1, 0,
        //	0.5f, -0.5f, 0,		0, 0, 1,
        //};

        LoadedMesh mesh;
        bool success = AssetManager::LoadMesh("Bunny.fbx", &mesh);

        //m_Mesh = new Mesh("Triangle", vertex_data, 8, _countof(vertex_data), index_data, _countof(index_data));
        //m_Mesh = new Mesh("Bunny.fbx");
        m_Material = new Material("TheRock.png", TextureColorSpace::sRGB);

        Scene* scene = Application::GetInstance()->GetScene();

        for (int i = 0; i < mesh.models.size(); i++)
        {
            m_Mesh = new Mesh("Mesh", mesh.models[i]);

            GameObject* object = scene->CreateGameObject();
            object->AddComponent<MeshRenderer>(m_Mesh, m_Material);
            Transform* t = object->AddComponent<Transform>();
            t->position = Float3(0.0f, 0.0f, 600.0f);
            t->rotation = Float3(0.0f, 180.0f, 0.0f);
            t->scale = Float3(1.0f);
            
            m_Transform = t;
        }


        void* window_handle0 = Application::GetInstance()->GetWindow(0)->GetNativeWindowHandle();
        //void* window_handle1 = Application::GetInstance()->GetWindow(1)->GetNativeWindowHandle();


        m_Obj1 = scene->CreateGameObject();
        m_Camera = m_Obj1->AddComponent<Transform>();
        Camera* cam_comp = m_Obj1->AddComponent<Camera>(0.01f, 1000.0f, 70.0f, window_handle0);
        cam_comp->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.5f });

        // UI
        {
            auto ui = scene->CreateGameObject();
            ui->AddComponent<Rect2D>(200.0f, 200.0f);
            auto ui_t = ui->AddComponent<Transform>();
            ui_t->position.x = 100.0f;
            ui_t->position.y = 100.0f;

            LoadedFont font;
            AssetManager::LoadFont("TypoGraphica.otf", &font, 48);

            m_Font = new Font("Cool Font", font);

            auto text = scene->CreateGameObject();
            text->AddComponent<Text2D>(m_Font, "Hoi Sylvia", 0.3f, 0.3f);
            auto text_t = text->AddComponent<Transform>();
            text_t->position.x = 0.5f;
            text_t->position.y = 50.0f;
        }

        //m_Obj2 = scene->CreateGameObject();
        //m_Obj2->AddComponent<Transform>();
        //Camera* cam_comp2 = m_Obj2->AddComponent<Camera>(0.01f, 1000.0f, 90.0f, window_handle1);
        //cam_comp2->SetClearColor({ 1.0f, 0.3f, 0.3f, 0.0f });
    }

    void OnUpdate(float delta) override
    {
        if (IsKeyDown(KeyCode::Q))
        {
            m_Transform->rotation.y += 25.0f * delta;
        }
        if (IsKeyDown(KeyCode::E))
        {
            m_Transform->rotation.x += 25.0f * delta;
        }
        
        static Float2 last_pos = GetMousePos();
        Float2 new_pos = GetMousePos();

        if (IsMouseKeyDown(MouseCode::ButtonRight))
        {
            Float2 vel = (new_pos - last_pos) * 0.2f;
            m_Camera->rotation.x += vel.y;
            m_Camera->rotation.y += vel.x;
        }

        last_pos = new_pos;

        m_Camera->rotation.x = Clamp(m_Camera->rotation.x, -89.0f, 89.0f);

        float pitch = DegreesToRadians(-m_Camera->rotation.x);
        float yaw   = DegreesToRadians(m_Camera->rotation.y);

        Float3 worldUp = Math::WorldUp;

        Float3 forward(
            Cos(pitch) * Sin(yaw),
            Sin(pitch),
            Cos(pitch) * Cos(yaw)
        );

        Float3 right = Float3::Cross(worldUp, forward);
        right.Normalize();

        Float3 up = Float3::Cross(forward, right);


        // Move forward/backward
        if (IsKeyDown(KeyCode::W))
            m_Camera->position = m_Camera->position + (forward * (250 * delta));
        if (IsKeyDown(KeyCode::S))
            m_Camera->position = m_Camera->position - (forward * (250 * delta));
        
        // Strafe left/right
        if (IsKeyDown(KeyCode::A))
            m_Camera->position = m_Camera->position - (right * (250 * delta));
        if (IsKeyDown(KeyCode::D))
            m_Camera->position = m_Camera->position + (right * (250 * delta));

        // Move up/down
        if (IsKeyDown(KeyCode::Space))
            m_Camera->position = m_Camera->position + (up * (250 * delta));
        if (IsKeyDown(KeyCode::LeftShift))
            m_Camera->position = m_Camera->position - (up * (250 * delta));


        //RB_LOG("Pos: %f, %f, %f", m_Camera->position.x, m_Camera->position.y, m_Camera->position.z);
    }

    bool OnEvent(const Event& event) override
    {
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