#include "Utils.h"
#include <Windows.h>

std::wstring ConvertAnsiToWide(const std::string& str)
{
	int count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), NULL, 0);
	std::wstring wstr(count, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), &wstr[0], count);
	return wstr;
}

void WcharToChar(const wchar_t* in_char, char* out_char)
{
	const size_t cSize = wcslen(in_char) + 1;
	memset(out_char, 0, cSize);
	wcstombs(out_char, in_char, cSize);
}
