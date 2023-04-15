#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>

#include "app/Application.h"
extern RB::Application* RB::CreateApplication();

#include "TempWindow.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    auto* app = RB::CreateApplication();

    char msgbuf[100];
    sprintf_s(msgbuf, "Application test value: %d\n", app->TestValue());
    OutputDebugStringA(msgbuf);

    delete app;

    while(true) {}

    //return TempWindow::win(hInstance, nCmdShow);
}