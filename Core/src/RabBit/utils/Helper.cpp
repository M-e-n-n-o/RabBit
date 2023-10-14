#include "Helper.h"

namespace RB
{
	const wchar_t* CharToWchar(const char* c)
	{
		const size_t cSize = strlen(c) + 1;
		wchar_t* wc = new wchar_t[cSize];
		mbstowcs(wc, c, cSize);

		return wc;
	}
}