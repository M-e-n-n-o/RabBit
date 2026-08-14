#pragma once

#ifdef RB_CONFIG_DEBUG
#ifdef RB_PLATFORM_WINDOWS
    #define RB_DEBUGGER_ATTACHED IsDebuggerPresent()
#else
    #define RB_DEBUGGER_ATTACHED false
#endif

    #define RB_ENABLE_ASSERTS
    #define RB_ENABLE_LOGS
    #define RB_DEBUG_BREAK if(RB_DEBUGGER_ATTACHED) __debugbreak()
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

    