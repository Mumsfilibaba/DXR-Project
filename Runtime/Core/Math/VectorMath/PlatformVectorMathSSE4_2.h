#pragma once
#if PLATFORM_SUPPORT_SSE4_2_INTRIN
#include "Core/Math/VectorMath/PlatformVectorMathSSE4_1.h"

struct FPlatformVectorMathSSE4_2 : public FPlatformVectorMathSSE4_1
{
};

#endif