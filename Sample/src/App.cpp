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
	app_info.appName		= "RabBit App";

	AppInfo::Window window1 = {};
	window1.windowName		= "Window 1";
	window1.windowWidth		= 1280;
	window1.windowHeight	= 720;
	app_info.windows.push_back(window1);

	AppInfo::Window window2 = {};
	window2.windowName		= "Window 2";
	window2.fullscreen		= true;
	window2.semiTransparent = true;
	app_info.windows.push_back(window2);

	return new App(app_info);
}