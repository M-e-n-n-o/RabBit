#include "RabBitCommon.h"
#include "String.h"

namespace RB
{
#if RB_PLATFORM_WINDOWS
    void CharToWchar(const char* inChar, wchar_t* outChar)
    {
        const size_t cSize = strlen(inChar) + 1;
        memset(outChar, 0, cSize);
        mbstowcs(outChar, inChar, cSize);
    }

    void WcharToChar(const wchar_t* inChar, char* outChar)
    {
        const size_t cSize = wcslen(inChar) + 1;
        memset(outChar, 0, cSize);
        wcstombs(outChar, inChar, cSize);
    }

    std::wstring CharToWString(const char* inChar)
    {
        if (!inChar)
            return {};

        int size = MultiByteToWideChar(
            CP_UTF8, 0,
            inChar, -1,
            nullptr, 0);

        std::wstring result(size - 1, L'\0');

        MultiByteToWideChar(
            CP_UTF8, 0,
            inChar, -1,
            result.data(), size);

        return result;
    }
#endif
}