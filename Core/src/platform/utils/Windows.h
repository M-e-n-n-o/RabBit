#if RB_PLATFORM_WINDOWS

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(max)
#undef max
#endif

#if defined(min)
#undef min
#endif

#if defined(FindWindow)
#undef FindWindow
#endif

#if defined(SendMessage)
#undef SendMessage
#endif

namespace RB
{
    // Custom graphics pointer
    template<class T>
    using GPtr = Microsoft::WRL::ComPtr<T>;
}
#endif