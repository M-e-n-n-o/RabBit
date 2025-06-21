#pragma once

#ifdef _WIN32
    #define RB_PLATFORM_WINDOWS
    
    #ifdef RB_CORE_ACCESS
        //#define RABBIT_API __declspec(dllexport)
    #else
        //#define RABBIT_API __declspec(dllimport)
    #endif
    
    #ifdef RB_CONFIG_DEBUG
        #define RB_ENABLE_ASSERTS
        #define RB_ENABLE_LOGS
        #define RB_DEBUG_BREAK __debugbreak()
    #endif
    
    #ifdef RB_CONFIG_OPTIMIZED
        #define RB_ENABLE_ASSERTS
        #define RB_ENABLE_LOGS
        #define RB_DEBUG_BREAK
    #endif
    
    #ifdef RB_CONFIG_DIST
        #define RB_DEBUG_BREAK
    #endif

    #define RB_LINE_STR     __LINE__
    #define RB_FUNCTION_STR __FUNCTION__

#else
    #error Make sure to specify a supported platform in the CMakeLists.txt! RabBit only supports Windows!
#endif
