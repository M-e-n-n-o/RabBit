#pragma once
#include <iostream>
#include <string>

#define LOG(message) std::wcout << message << std::endl

#define EXIT_ON_FAIL_HR(hresult, error_message)			\
		if (FAILED(hresult)) {							\
			std::wcout << error_message << std::endl;	\
			std::exit(-1);								\
		}

#define EXIT_ON_FAIL(result, error_message)				\
		if (!(result)) {								\
			std::wcout << error_message << std::endl;	\
			std::exit(-1);								\
		}

std::wstring ConvertAnsiToWide(const std::string & str);
void WcharToChar(const wchar_t* in_char, char* out_char);