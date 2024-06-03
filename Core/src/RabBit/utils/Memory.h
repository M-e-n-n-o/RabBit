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

#if defined(FindWindow)
#undef FindWindow
#endif

#include <memory>

namespace RB
{
	#define ALLOC_STACK(size)		alloca((size))
	#define ALLOC_HEAP(size)		malloc((size))

	#define SAFE_RELEASE(obj)		(obj)->Release();
	#define SAFE_DELETE(obj)		if ((obj) != nullptr) { delete (obj); (obj) = nullptr; }
	#define SAFE_FREE(obj)			if ((obj) != nullptr) { free(obj); (obj) = nullptr; }

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

	template<typename T, typename T2>
	constexpr Shared<T> CastShared(Shared<T2> base)
	{
		return std::dynamic_pointer_cast<T>(base);
	}

	// Custom unique pointer
	template<typename T>
	using Unique = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Unique<T> CreateUnique(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
}