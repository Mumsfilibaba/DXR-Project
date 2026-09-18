#ifndef CLEAR_ELEMENT_UINT
    #define CLEAR_ELEMENT_UINT 0
#endif

#ifndef CLEAR_ELEMENT_SINT
    #define CLEAR_ELEMENT_SINT 0
#endif

#define NUM_THREADS (64)

[[vk::push_constant]]
struct FShaderBlockConstants
{
    uint4 ClearValue;
    uint  NumElements;
} Constants;

#if CLEAR_ELEMENT_UINT
    [[vk::image_format("unknown")]] RWBuffer<uint4> OutputBuffer : register(u0);
    #define CLEAR_ELEMENT_VALUE (Constants.ClearValue)
#elif CLEAR_ELEMENT_SINT
    [[vk::image_format("unknown")]] RWBuffer<int4> OutputBuffer : register(u0);
    #define CLEAR_ELEMENT_VALUE (asint(Constants.ClearValue))
#else
    [[vk::image_format("unknown")]] RWBuffer<float4> OutputBuffer : register(u0);
    #define CLEAR_ELEMENT_VALUE (asfloat(Constants.ClearValue))
#endif

[numthreads(NUM_THREADS, 1, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x < Constants.NumElements)
    {
        OutputBuffer[DispatchThreadID.x] = CLEAR_ELEMENT_VALUE;
    }
}
