#include "Structs.hlsli"
#include "Constants.hlsli"

RWTexture2D<uint> Output : register(u0, space0);

[numthreads(1, 1, 1)]
void Main(ComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
    
    int2 TexCoord = int2(Input.DispatchThreadID.xy);
    Output[TexCoord] = SHADING_RATE_2x2;
}