#pragma once

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

    