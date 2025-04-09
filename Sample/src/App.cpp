#include <RabBit.h>
#include "TestLayer.h"

using namespace RB;
using namespace RB::Events;
using namespace RB::Entity;
using namespace RB::Math;

class App : public RB::Application
{
public:
    App(RB::AppInfo& info) : Application(info) {}

    void OnStart() override
    {
        PushLayer<TestLayer>();
    }

    void OnStop() override
    {
    }
};

RB::Application* RB::CreateApplication(const char* launch_args)
{
    AppInfo app_info = {};
    app_info.appName            = "RabBit App";

    AppInfo::Window window1 = {};
    window1.windowName          = "Window 1";
    window1.windowWidth         = 1280;
    window1.windowHeight        = 720;
    //window1.forcedRenderAspect  = 4.0f / 3.0f;
    //window1.renderScale         = 0.25f;
    window1.semiTransparent     = true;
    //app_info.windows.push_back(window1);

    AppInfo::Window window2 = {};
    window2.windowName          = "Window 2";
    //window2.fullscreen          = true;
    //window2.semiTransparent     = true;
    window2.windowWidth         = 1280;
    window2.windowHeight        = 720;
    window2.forcedRenderAspect  = 0.0f;
    window2.renderScale         = 1.0f;
    app_info.windows.push_back(window2);

    return new App(app_info);
}