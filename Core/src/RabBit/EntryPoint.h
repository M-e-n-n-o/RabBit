#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "utils/debug/Log.h"
#include "graphics/NativeWindow.h"
#include "app/Application.h"

extern RB::Application* RB::CreateApplication();

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Create the logger
#ifdef RB_ENABLE_LOGS
    RB::Debug::Logger::OpenConsole();
#endif

    RB::Graphics::NativeWindow::RegisterWindowCLass(hInstance, L"DX12WindowClass");
    RB::Graphics::NativeWindow::CreateWindow(hInstance, L"DX12WindowClass", L"RabBit App", 1920, 1080);
    RB::Graphics::NativeWindow::ShowWindow();

    auto* app = RB::CreateApplication();

    app->Run();

    delete app;

    return 0;
}