// Custom ImGui config file
#pragma once

#include "rabbit/utils/debug/Assert.h"

// Custom assert so that the application doesn't close after asserting
#define IM_ASSERT(_EXPR) do { RB_ASSERT(_EXPR, "ImGui assert failed") } while (0)

// Custom ImGuiContext per thread so that main and render thread don't interfere with each other
// (defined in ImGuiManager.cpp)
struct ImGuiContext;
extern thread_local ImGuiContext* g_CustomImGuiTLS;
#define GImGui g_CustomImGuiTLS