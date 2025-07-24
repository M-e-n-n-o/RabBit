#pragma once

namespace RB
{
    void CharToWchar(const char* inChar, wchar_t* outChar);

    void WcharToChar(const wchar_t* inChar, char* outChar);

    //struct String
    //{
    //public:
    //	String(char* string);
    //	String(wchar_t* string);

    //	wchar_t* GetWCharArray() const;
    //	char* GetCharArray() const;

    //private:
    //	union
    //	{
    //		char*	 m_Char;
    //		wchar_t* m_WChar;
    //	};

    //	bool m_InitWithChar;
    //};
}