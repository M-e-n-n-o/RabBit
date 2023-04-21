#include "Core.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef RB_ENABLE_ASSERTS
#define RB_LOG(...) { char msg[500]; sprintf(msg, __VA_ARGS__);  OutputDebugStringA(msg); }
#else
#define RB_LOG(x)
#endif