#include "Structs.hlsli"
#include "Constants.hlsli"

RWTexture2D<uint> Output : register(u0, space0);

[numthreads(1, 1, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoord   = Input.DispatchThreadID.xy;
    Output[TexCoord] = SHADING_RATE_4x4;
}