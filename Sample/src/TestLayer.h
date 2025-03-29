#include <RabBit.h>

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;

class TestLayer : public ApplicationLayer
{
private:
    Mesh* m_Mesh;
    Material* m_Material;

    GameObject* m_Obj1;
    GameObject* m_Obj2;

    Transform* m_Transform;
    Transform* m_Camera;

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

        //m_Mesh = new Mesh("Triangle", vertex_data, 8, _countof(vertex_data), index_data, _countof(index_data));
        m_Mesh = new Mesh("Bunny.fbx");
        m_Material = new Material("TheRock.png");

        Scene* scene = Application::GetInstance()->GetScene();

        GameObject* object = scene->CreateGameObject();
        object->AddComponent<MeshRenderer>(m_Mesh, m_Material);
        Transform* t = object->AddComponent<Transform>();
        t->position = Float3(0.0f, 0.0f, 5.0f);
        t->rotation = Float3(45.0f, 0.0f, 0.0f);
        t->scale = Float3(1.0f);

        m_Transform = t;

        m_Obj1 = scene->CreateGameObject();
        m_Camera = m_Obj1->AddComponent<Transform>();
        Camera* cam_comp = m_Obj1->AddComponent<Camera>(0.01f, 1000.0f, 90.0f, 0);
        cam_comp->SetClearColor({ 0.0f, 0.3f, 0.3f, 0.5f });

        m_Obj2 = scene->CreateGameObject();
        m_Obj2->AddComponent<Transform>();
        Camera* cam_comp2 = m_Obj2->AddComponent<Camera>(0.01f, 1000.0f, 90.0f, 1);
        cam_comp2->SetClearColor({ 1.0f, 0.3f, 0.3f, 0.0f });
    }

    void OnUpdate() override
    {
        //RB_LOG("Test");

        if (IsMouseKeyDown(MouseCode::ButtonLeft))
        {
            m_Transform->rotation.y += 0.001f;
        }
        if (IsMouseKeyDown(MouseCode::ButtonRight))
        {
            m_Transform->rotation.x += 0.001f;
        }

        if (IsKeyDown(KeyCode::W))
        {
            m_Camera->position.z += 0.001f;
        }
        if (IsKeyDown(KeyCode::A))
        {
            m_Camera->position.x -= 0.001f;
        }
        if (IsKeyDown(KeyCode::S))
        {
            m_Camera->position.z -= 0.001f;
        }
        if (IsKeyDown(KeyCode::D))
        {
            m_Camera->position.x += 0.001f;
        }
        if (IsKeyDown(KeyCode::LeftShift))
        {
            m_Camera->rotation.x += 0.001f;
        }
        if (IsKeyDown(KeyCode::Space))
        {
            m_Camera->rotation.x -= 0.001f;
        }
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
    }
};