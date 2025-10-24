//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <cwchar>
#include <exception>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <map>
#include <queue>
#include <deque>
#include <array>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <typeindex>
#include <utility>

#include "Core.h"

#include "utils/Util.h"
#include "utils/String.h"
#include "utils/Container.h"
#include "utils/Memory.h"
#include "utils/Threading.h"

#include "utils/debug/Log.h"
#include "utils/debug/Assert.h"

#include "math/Misc.h"
#include "math/Vector.h"
#include "math/Matrix.h"

#if RB_PLATFORM_WINDOWS
#include "platform/utils/Windows.h"
#endif
