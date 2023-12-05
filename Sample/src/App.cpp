#include <RabBit.h>

class App : public RB::Application
{
public:
	App(RB::AppInfo& info): Application(info) {}

	void OnStart() override
	{
		RB_LOG("Hoiii");
	}

	void OnUpdate() override
	{
		//RB_LOG("Test");
	}

	void OnStop() override
	{

	}
};

RB::Application* RB::CreateApplication(const char* launch_args)
{
	RB::AppInfo app_info	= {};
	app_info.name			= "RabBit App";
	app_info.windowWidth	= 1920;
	app_info.windowHeight	= 1080;

	return new App(app_info);
}