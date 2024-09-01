#include "RabBitCommon.h"
#include "String.h"

namespace RB
{
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
}