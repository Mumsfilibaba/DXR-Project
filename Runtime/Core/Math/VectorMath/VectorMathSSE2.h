#pragma once
#if PLATFORM_SUPPORT_SSE2_INTRIN
#include "Core/Math/VectorMath/VectorMathSSE.h"

#if PLATFORM_WINDOWS
    #include <emmintrin.h> // SSE2
#elif PLATFORM_MACOS
    #include <emmintrin.h> // SSE2
#else
    #error "No valid platform. This code requires SSE2 support on Windows or macOS."
#endif

struct FVectorMathSSE2 : public FVectorMathSSE
{
};

#endif
