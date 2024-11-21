#pragma once
#if PLATFORM_SUPPORT_SSE4_1_INTRIN
#include "Core/Math/VectorMath/VectorMathSSSE3.h"

struct FVectorMathSSE4_1 : public FVectorMathSSSE3
{
};

#endif