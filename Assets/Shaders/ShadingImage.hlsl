#include "Structs.hlsli"
#include "Constants.hlsli"

RWTexture2D<uint> Output : register(u0);

[numthreads(1, 1, 1)]
void Main(FComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
    
    int2   TexCoord = int2(Input.DispatchThreadID.xy);
    float2 Center   = float2(Width / 2, Height / 2);
    float  Distance = length(float2(TexCoord) - Center);
    
    if (Distance > 40.0f)
    {
        Output[TexCoord] = SHADING_RATE_4x4;
    }
    else if (Distance > 12.0f)
    {
        Output[TexCoord] = SHADING_RATE_2x2;
    }
    else
    {
        Output[TexCoord] = SHADING_RATE_1x1;
    }
}