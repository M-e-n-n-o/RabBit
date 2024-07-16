#include <RabBit.h>

using namespace RB;
using namespace RB::Input;
using namespace RB::Entity;
using namespace RB::Math;

class App : public RB::Application
{
private:
	Mesh*		m_Mesh;
	Material*	m_Material;

	Transform*	m_Transform;
	Transform*	m_Camera;

public:
	App(RB::AppInfo& info): Application(info) {}

	void OnStart() override
	{
		RB_LOG("Hoiii");

		float vertex_data[] = {
			// Pos					Color
			-1.0f,  -1.0f, -1.0f,	0.0f, 0.0f, 0.0f, // 0
			-1.0f,   1.0f, -1.0f,	0.0f, 1.0f, 0.0f, // 1
			 1.0f,   1.0f, -1.0f,	1.0f, 1.0f, 0.0f, // 2
			 1.0f,  -1.0f, -1.0f,	1.0f, 0.0f, 0.0f, // 3
			-1.0f,  -1.0f,  1.0f,	0.0f, 0.0f, 1.0f, // 4
			-1.0f,   1.0f,  1.0f,	0.0f, 1.0f, 1.0f, // 5
			 1.0f,   1.0f,  1.0f,	1.0f, 1.0f, 1.0f, // 6
			 1.0f,  -1.0f,  1.0f,	1.0f, 0.0f, 1.0f  // 7
		};

		uint16_t index_data[] = {
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

		m_Mesh = new Mesh("Triangle", vertex_data, 6, _countof(vertex_data), index_data, _countof(index_data));
		m_Material = new Material("TheRock.png");

		GameObject* object = GetScene()->CreateGameObject();
		object->AddComponent<MeshRenderer>(m_Mesh, m_Material);
		Transform* t = object->AddComponent<Transform>();
		t->position = Float3(0.0f, 0.0f, 5.0f);
		t->rotation = Float3(45.0f, 0.0f, 0.0f);
		t->scale	= Float3(1.0f);

		m_Transform = t;

		GameObject* camera = GetScene()->CreateGameObject();
		camera->AddComponent<Camera>(0.01f, 1000.0f, 90.0f, 0);
		m_Camera = camera->AddComponent<Transform>();
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
			m_Camera->position.z += 0.0001f;
		}
		if (IsKeyDown(KeyCode::A))
		{
			m_Camera->position.x -= 0.0001f;
		}
		if (IsKeyDown(KeyCode::S))
		{
			m_Camera->position.z -= 0.0001f;
		}
		if (IsKeyDown(KeyCode::D))
		{
			m_Camera->position.x += 0.0001f;
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

	void OnStop() override
	{
		delete m_Mesh;
		delete m_Material;
	}
};

RB::Application* RB::CreateApplication(const char* launch_args)
{
	AppInfo app_info = {};
	app_info.appName		= "RabBit App";

	AppInfo::Window window1 = {};
	window1.windowName		= "Window 1";
	window1.windowWidth		= 1280;
	window1.windowHeight	= 720;
	app_info.windows.push_back(window1);

	//AppInfo::Window window2 = {};
	//window2.windowName		= "Window 2";
	//window2.fullscreen		= true;
	//window2.semiTransparent = true;
	//app_info.windows.push_back(window2);

	return new App(app_info);
}