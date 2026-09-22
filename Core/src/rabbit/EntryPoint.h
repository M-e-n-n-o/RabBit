#pragma once

#include "utils/String.h"
#include "utils/debug/Log.h"
#include "events/Event.h"
#include "app/Application.h"

extern RB::Application* RB::CreateApplication(int argc, char** argv);

int main(int argc, char* argv[])
{
    // Create the logger
#ifdef RB_ENABLE_LOGS
    RB::Utils::Debug::Logger::OpenConsole();

    RB::Utils::Debug::Logger::LogTime();
    RB::Utils::Debug::Logger::LogCore(RB::LOGTAG_MAIN, "Welcome to the RabBit Engine\n");
#endif

    RB::Events::g_EventManager = new RB::Events::EventManager();

    auto* app = RB::CreateApplication(argc, argv);

    bool success = app->Start(argc, argv);

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