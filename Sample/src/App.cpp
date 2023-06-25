#include <RabBit.h>

class App : public RB::Application
{
public:
	App(RB::AppInfo& info): Application(info) {}

	void Start() override
	{
		RB_LOG("t", "Hoiii");
	}

	void Update() override
	{

	}

	void Stop() override
	{

	}
};

RB::Application* RB::CreateApplication()
{
	RB::AppInfo app_info	= {};
	app_info.name			= L"RabBit App";
	app_info.windowWidth	= 1920;
	app_info.windowHeight	= 1080;

	return new App(app_info);
}