#include <RabBit.h>

using namespace RB;
using namespace RB::Input;
using namespace RB::Entity;
using namespace RB::Math;

class App : public RB::Application
{
private:
	Mesh* m_Mesh;

	Transform* m_Transform;

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

		m_Mesh = new Mesh("Triangle", vertex_data, 6, _countof(vertex_data), index_data, _countof(index_data));

		GameObject* object = GetScene()->CreateGameObject();
		object->AddComponent<MeshRenderer>(m_Mesh);
		m_Transform = object->AddComponent<Transform>();
		m_Transform->position = Float3(0.0f, 0.0f, 100.0f);
		m_Transform->scale	= Float3(0.005f);

		GameObject* camera = GetScene()->CreateGameObject();
		camera->AddComponent<Camera>(0.01f, 1000.0f, 90.0f, 0);
		camera->AddComponent<Transform>();

		static_assert(false);
		/*
			Things to fix:
			- Visualize cube
				- Something with the projection matrix and the depth is going wrong I think
				- The vertex/index data is not completely correct
			- Small memory leak somewhere
		*/
	}

	void OnUpdate() override
	{
		//RB_LOG("Test");

		if (IsMouseKeyDown(MouseCode::ButtonLeft))
		{
			m_Transform->position.z += 0.00001f;

		}
	}

	void OnStop() override
	{
		delete m_Mesh;
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