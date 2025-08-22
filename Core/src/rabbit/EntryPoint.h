#pragma once

#include "utils/String.h"
#include "utils/debug/Log.h"
#include "events/Event.h"
#include "app/Application.h"

extern RB::Application* RB::CreateApplication(const char* launch_args);

int main(int argc, char* argv[])
{
    // Create the logger
#ifdef RB_ENABLE_LOGS
    RB::Utils::Debug::Logger::OpenConsole();
#endif

    RB::Events::g_EventManager = new RB::Events::EventManager();

    std::string args;
    for (int argi = 0; argi < argc; ++argi)
    {
        args += argv[argi];
        args += " ";
    }

    auto* app = RB::CreateApplication(args.c_str());

    bool success = app->Start(args.c_str());

    if (success)
    {
        app->Run();
        app->Shutdown();
    }
    else
    {
        RB_LOG_CRITICAL("Failed to start application, exiting...");
    }

    delete app;
    delete RB::Events::g_EventManager;

    return 0;
}