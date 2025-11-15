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
        //bool success = AssetManager::LoadMesh("Sponza/source/Sponza.fbx", &mesh);
        bool success = AssetManager::LoadMesh("Bunny.fbx", &mesh);

        //m_Mesh = new Mesh("Triangle", vertex_data, 8, _countof(vertex_data), index_data, _countof(index_data));
        m_Material = new Material("TheRock.png", TextureColorSpace::sRGB);

        Scene* scene = Application::GetInstance()->GetScene();

        for (int i = 0; i < mesh.models.size(); i++)
        {
            m_Mesh = new Mesh("Mesh", mesh.models[i]);

            GameObject* object = scene->CreateGameObject();
            object->AddComponent<MeshRenderer>(m_Mesh, m_Material);
            Transform* t = object->AddComponent<Transform>();
            t->position = mesh.models[i].position + Math::Float3(0, 0, 50);
            t->rotation = mesh.models[i].rotation;
            t->scale = Float3(0.1f);
            
            m_Transform = t;
        }


        void* window_handle0 = Application::GetInstance()->GetWindow(0)->GetNativeWindowHandle();
        //void* window_handle1 = Application::GetInstance()->GetWindow(1)->GetNativeWindowHandle();


        m_Obj1 = scene->CreateGameObject();
        m_Camera = m_Obj1->AddComponent<Transform>();
        Camera* cam_comp = m_Obj1->AddComponent<Camera>(0.01f, 1000.0f, 70.0f, window_handle0);
        cam_comp->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.4f });

        auto* sun = scene->CreateGameObject();
        sun->AddComponent<DirectionalLight>(Math::Float3(-0.2f, -0.98f, 0.0f), Math::Float3(0.99f, 0.97f, 0.76f));

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
            m_Camera->position = m_Camera->position + (forward * (100 * delta));
        if (IsKeyDown(KeyCode::S))
            m_Camera->position = m_Camera->position - (forward * (100 * delta));
        
        // Strafe left/right
        if (IsKeyDown(KeyCode::A))
            m_Camera->position = m_Camera->position - (right * (100 * delta));
        if (IsKeyDown(KeyCode::D))
            m_Camera->position = m_Camera->position + (right * (100 * delta));

        // Move up/down
        if (IsKeyDown(KeyCode::Space))
            m_Camera->position = m_Camera->position + (up * (100 * delta));
        if (IsKeyDown(KeyCode::LeftShift))
            m_Camera->position = m_Camera->position - (up * (100 * delta));


        //RB_LOG("Pos: %f, %f, %f", m_Camera->position.x, m_Camera->position.y, m_Camera->position.z);
        //RB_LOG("Rot: %f, %f, %f", m_Camera->rotation.x, m_Camera->rotation.y, m_Camera->rotation.z);
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