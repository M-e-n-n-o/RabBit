#include "Core.h"

#define RB_ASSERT_RELEASE(x) if(!x) throw std::exception();

#ifdef RB_ENABLE_ASSERTS

#define RB_ASSERT(x) if(!(x)) throw std::exception();
#define RB_ASSERT_D3D(x) if(FAILED(x)) throw std::exception();

#else

#define RB_ASSERT(x)
#define RB_ASSERT_D3D(x)

#endif