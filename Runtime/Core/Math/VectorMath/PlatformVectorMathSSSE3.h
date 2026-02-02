#pragma once
#if PLATFORM_SUPPORT_SSSE3_INTRIN
#include "Core/Math/VectorMath/PlatformVectorMathSSE3.h"

#if PLATFORM_WINDOWS
    #include <tmmintrin.h> // SSSE3
#elif PLATFORM_MACOS
    #include <tmmintrin.h> // SSSE3
#else
    #error "No valid platform. This code requires SSSE3 support on Windows or macOS."
#endif

struct FPlatformVectorMathSSSE3 : public FPlatformVectorMathSSE3
{
};

#endif