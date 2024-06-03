#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "utils/String.h"
#include "utils/debug/Log.h"
#include "input/events/Event.h"
#include "app/Application.h"

extern RB::Application* RB::CreateApplication(const char* launch_args);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Create the logger
#ifdef RB_ENABLE_LOGS
    RB::Utils::Debug::Logger::OpenConsole();
#endif

    RB::Input::Events::g_EventManager = new RB::Input::Events::EventManager();

    char args[100];
    RB_ASSERT_FATAL(_countof(args) >= (wcslen(lpCmdLine) + 1), "Launch arguments copy should be made longer!");
    RB::WcharToChar(lpCmdLine, args);

    auto* app = RB::CreateApplication(args);

    app->Start(args);

    app->Run();

    app->Shutdown();

    delete app;
    delete RB::Input::Events::g_EventManager;

    return 0;
}