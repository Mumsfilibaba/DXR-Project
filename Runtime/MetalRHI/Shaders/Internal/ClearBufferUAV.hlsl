#include "CoreDefines.hlsli"

#ifndef CLEAR_ELEMENT_UINT
    #define CLEAR_ELEMENT_UINT 0
#endif

#ifndef CLEAR_ELEMENT_SINT
    #define CLEAR_ELEMENT_SINT 0
#endif

#ifndef CLEAR_ELEMENT_UNTYPED
    #define CLEAR_ELEMENT_UNTYPED 0
#endif

#define NUM_THREADS (64)

SHADER_CONSTANT_BLOCK_BEGIN
    uint4 ClearValue;
    uint  NumElements;
SHADER_CONSTANT_BLOCK_END

#if CLEAR_ELEMENT_UNTYPED
    RWByteAddressBuffer OutputBuffer : register(u0);
#elif CLEAR_ELEMENT_UINT
    TEXTURE_FORMAT_UNKNOWN RWBuffer<uint4> OutputBuffer : register(u0);
    #define CLEAR_ELEMENT_VALUE (Constants.ClearValue)
#elif CLEAR_ELEMENT_SINT
    TEXTURE_FORMAT_UNKNOWN RWBuffer<int4> OutputBuffer : register(u0);
    #define CLEAR_ELEMENT_VALUE (asint(Constants.ClearValue))
#else
    TEXTURE_FORMAT_UNKNOWN RWBuffer<float4> OutputBuffer : register(u0);
    #define CLEAR_ELEMENT_VALUE (asfloat(Constants.ClearValue))
#endif

[numthreads(NUM_THREADS, 1, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x < Constants.NumElements)
    {
    #if CLEAR_ELEMENT_UNTYPED
        OutputBuffer.Store(DispatchThreadID.x * 4, Constants.ClearValue.x);
    #else
        OutputBuffer[DispatchThreadID.x] = CLEAR_ELEMENT_VALUE;
    #endif
    }
}
