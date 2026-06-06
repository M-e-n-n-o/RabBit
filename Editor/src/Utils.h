#pragma once

#include <string>
#include <cstdarg>

namespace Editor
{
    std::string FormatToString(const char* format, va_list args)
    {
        // Make a copy of args because vsnprintf consumes the va_list
        va_list args_copy;
        va_copy(args_copy, args);

        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size <= 0)
        {
            return std::string();
        }

        std::string result;
        result.resize(size);

        vsnprintf(&result[0], size + 1, format, args);

        return result;
    }
}