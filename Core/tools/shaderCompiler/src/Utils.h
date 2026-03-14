#pragma once
#include <iostream>
#include <string>

#define LOGW(message) std::wcout << message << std::endl
#define LOG(message) std::cout << message << std::endl

#define EXIT_ON_FAIL_HR(hresult, error_message)			\
		if (FAILED(hresult)) {							\
			std::wcout << error_message << std::endl;	\
			std::exit(-1);								\
		}

#define EXIT_ON_FAIL_SL(result, error_message)			\
		if (SLANG_FAILED(result)) {						\
			std::wcout << error_message << std::endl;	\
			std::exit(-1);								\
		}

#define EXIT_ON_FAIL(result, error_message)				\
		if (!(result)) {								\
			std::wcout << error_message << std::endl;	\
			std::exit(-1);								\
		}