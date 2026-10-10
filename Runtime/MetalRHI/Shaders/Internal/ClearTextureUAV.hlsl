#ifndef CLEAR_ELEMENT_UINT
    #define CLEAR_ELEMENT_UINT 0
#endif

#ifndef CLEAR_ELEMENT_SINT
    #define CLEAR_ELEMENT_SINT 0
#endif

#define CLEAR_DIMENSION_1D       0
#define CLEAR_DIMENSION_1D_ARRAY 1
#define CLEAR_DIMENSION_2D       2
#define CLEAR_DIMENSION_2D_ARRAY 3
#define CLEAR_DIMENSION_3D       4

#ifndef CLEAR_DIMENSION
    #define CLEAR_DIMENSION CLEAR_DIMENSION_2D
#endif

[[vk::push_constant]]
struct FShaderBlockConstants
{
    uint4 ClearValue;
} Constants;

#if CLEAR_ELEMENT_UINT
    #define CLEAR_ELEMENT_TYPE  uint4
    #define CLEAR_ELEMENT_VALUE (Constants.ClearValue)
#elif CLEAR_ELEMENT_SINT
    #define CLEAR_ELEMENT_TYPE  int4
    #define CLEAR_ELEMENT_VALUE (asint(Constants.ClearValue))
#else
    #define CLEAR_ELEMENT_TYPE  float4
    #define CLEAR_ELEMENT_VALUE (asfloat(Constants.ClearValue))
#endif

#if CLEAR_DIMENSION == CLEAR_DIMENSION_1D
    [[vk::image_format("unknown")]] RWTexture1D<CLEAR_ELEMENT_TYPE> OutputTexture : register(u0);
#elif CLEAR_DIMENSION == CLEAR_DIMENSION_1D_ARRAY
    [[vk::image_format("unknown")]] RWTexture1DArray<CLEAR_ELEMENT_TYPE> OutputTexture : register(u0);
#elif CLEAR_DIMENSION == CLEAR_DIMENSION_2D
    [[vk::image_format("unknown")]] RWTexture2D<CLEAR_ELEMENT_TYPE> OutputTexture : register(u0);
#elif CLEAR_DIMENSION == CLEAR_DIMENSION_2D_ARRAY
    [[vk::image_format("unknown")]] RWTexture2DArray<CLEAR_ELEMENT_TYPE> OutputTexture : register(u0);
#else
    [[vk::image_format("unknown")]] RWTexture3D<CLEAR_ELEMENT_TYPE> OutputTexture : register(u0);
#endif

// The dispatch is sized from the texture desc, so every write is bounds-checked against the view itself
[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
#if CLEAR_DIMENSION == CLEAR_DIMENSION_1D
    uint Width;
    OutputTexture.GetDimensions(Width);
    if (DispatchThreadID.x < Width && DispatchThreadID.y == 0 && DispatchThreadID.z == 0)
    {
        OutputTexture[DispatchThreadID.x] = CLEAR_ELEMENT_VALUE;
    }
#elif CLEAR_DIMENSION == CLEAR_DIMENSION_1D_ARRAY
    uint Width;
    uint NumSlices;
    OutputTexture.GetDimensions(Width, NumSlices);
    if (DispatchThreadID.x < Width && DispatchThreadID.y == 0 && DispatchThreadID.z < NumSlices)
    {
        OutputTexture[DispatchThreadID.xz] = CLEAR_ELEMENT_VALUE;
    }
#elif CLEAR_DIMENSION == CLEAR_DIMENSION_2D
    uint Width;
    uint Height;
    OutputTexture.GetDimensions(Width, Height);
    if (DispatchThreadID.x < Width && DispatchThreadID.y < Height && DispatchThreadID.z == 0)
    {
        OutputTexture[DispatchThreadID.xy] = CLEAR_ELEMENT_VALUE;
    }
#elif CLEAR_DIMENSION == CLEAR_DIMENSION_2D_ARRAY
    uint Width;
    uint Height;
    uint NumSlices;
    OutputTexture.GetDimensions(Width, Height, NumSlices);
    if (DispatchThreadID.x < Width && DispatchThreadID.y < Height && DispatchThreadID.z < NumSlices)
    {
        OutputTexture[DispatchThreadID] = CLEAR_ELEMENT_VALUE;
    }
#else
    uint Width;
    uint Height;
    uint Depth;
    OutputTexture.GetDimensions(Width, Height, Depth);
    if (DispatchThreadID.x < Width && DispatchThreadID.y < Height && DispatchThreadID.z < Depth)
    {
        OutputTexture[DispatchThreadID] = CLEAR_ELEMENT_VALUE;
    }
#endif
}
