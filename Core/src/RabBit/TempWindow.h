#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Core.h"

class TempWindow
{
public:
	static int win(HINSTANCE hInstance, int nCmdShow);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static void ExitGame() noexcept;
};