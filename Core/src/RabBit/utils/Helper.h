#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(max)
#undef max
#endif

#if defined(min)
#undef min
#endif

#include <memory>

namespace RB
{
	#define ALLOC_STACK(size)		alloca(size)
	#define ALLOC_HEAP(size)		malloc(size)

	// Pointers
		// Custom graphics pointer
		template<class T>
		using GPtr = Microsoft::WRL::ComPtr<T>;

		// Custom shared pointer
		template<typename T>
		using Shared = std::shared_ptr<T>;
		template<typename T, typename ... Args>
		constexpr Shared<T> CreateShared(Args&& ... args)
		{
			return std::make_shared<T>(std::forward<Args>(args)...);
		}

		// Custom unique pointer
		template<typename T>
		using Unique = std::unique_ptr<T>;
		template<typename T, typename ... Args>
		constexpr Unique<T> CreateUnique(Args&& ... args)
		{
			return std::make_unique<T>(std::forward<Args>(args)...);
		}

	// Data containers
		template<class T>
		using List = std::vector<T>;

		template<class T1, class T2>
		using Map = std::map<T1, T2>;

		template<class T1, class T2>
		using UnorderedMap = std::unordered_map<T1, T2>;

	void CharToWchar(const char* inChar, wchar_t* outChar);

	void WcharToChar(const wchar_t* inChar, char* outChar);
}