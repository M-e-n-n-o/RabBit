#pragma once
#include <string>
#include "Log.h"

#define DEFINE_FIND_LAUNCH_ARG(argc, argv)                                                          \
    auto FindLaunchArg = [argc, argv](const char* requested_arg) -> const char*                     \
    {                                                                                               \
        for (int i = 0; i < argc; i++)                                                              \
        {                                                                                           \
            if (std::strcmp(requested_arg, argv[i]) == 0)                                           \
            {                                                                                       \
                EXIT_ON_FAIL(i + 1 < argc, "Not enough command line arguments specified");          \
                return argv[i + 1];                                                                 \
            }                                                                                       \
        }                                                                                           \
        return nullptr;                                                                             \
    };

#define DEFINE_HAS_LAUNCH_ARG(argc, argv)                                                           \
auto HasLaunchArg = [argc, argv](const char* requested_arg) -> bool                                 \
    {                                                                                               \
        for (int i = 0; i < argc; i++)                                                              \
        {                                                                                           \
            if (std::strcmp(requested_arg, argv[i]) == 0)                                           \
            {                                                                                       \
                return true;                                                                        \
            }                                                                                       \
        }                                                                                           \
        return false;                                                                               \
    };