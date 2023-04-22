#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include "utils/debug/Log.h"
#include "app/Application.h"

extern RB::Application* RB::CreateApplication();

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Create the logger
#ifdef RB_ENABLE_LOGS
    RB::Logger::OpenConsole();
#endif

    auto* app = RB::CreateApplication();

    app->Run();

    delete app;

    return 0;
}