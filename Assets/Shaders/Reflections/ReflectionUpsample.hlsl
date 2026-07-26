#include "Structs.hlsli"
#include "Helpers.hlsli"

Texture2D<float4> HalfResColor  : register(t0); // rgb = denoised color, a = variance (half res)
Texture2D<float4> GBufferNormal : register(t1); // rgb = world-space normal (packed)
Texture2D<float>  GBufferDepth  : register(t2); // r   = device depth

SamplerState PointSampler : register(s0);

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> Output : register(u0); // rgb = full-res upsampled reflection color

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    float2 FullSize;
    float2 HalfSize;
SHADER_CONSTANT_BLOCK_END

[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 Pixel = DispatchThreadID.xy;
    if (Pixel.x >= uint(Constants.FullSize.x) || Pixel.y >= uint(Constants.FullSize.y))
    {
        return;
    }

    const float2 TexCoord = (float2(Pixel) + 0.5f) / Constants.FullSize;

    const float CenterDepth = GBufferDepth[Pixel];
    if (CenterDepth >= 1.0f)
    {
        Output[Pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    const float3 CenterNormal = UnpackNormal(GBufferNormal[Pixel].rgb);
    const float2 HalfCoordF   = TexCoord * Constants.HalfSize - 0.5f;
    const int2   HalfBase     = int2(floor(HalfCoordF));

    float3 SumColor  = float3(0.0f, 0.0f, 0.0f);
    float  SumWeight = 0.0f;

    [unroll]
    for (int dy = 0; dy <= 1; ++dy)
    {
        [unroll]
        for (int dx = 0; dx <= 1; ++dx)
        {
            const int2   HalfCoord   = clamp(HalfBase + int2(dx, dy), int2(0, 0), int2(Constants.HalfSize) - 1);
            const float2 TapTexCoord = (float2(HalfCoord) + 0.5f) / Constants.HalfSize;
            const float  TapDepth    = GBufferDepth.SampleLevel(PointSampler, TapTexCoord, 0);
            const float3 TapNormal   = UnpackNormal(GBufferNormal.SampleLevel(PointSampler, TapTexCoord, 0).rgb);
            const float2 BilinearF   = 1.0f - abs(HalfCoordF - float2(HalfCoord));
            const float  WBilinear   = max(BilinearF.x, 0.0f) * max(BilinearF.y, 0.0f);
            const float  WDepth      = exp(-abs(CenterDepth - TapDepth) / 0.01f);
            const float  WNormal     = pow(max(0.0f, dot(CenterNormal, TapNormal)), 32.0f);
            const float  Weight      = WBilinear * WDepth * WNormal + 1e-5f;

            SumColor  += HalfResColor[HalfCoord].rgb * Weight;
            SumWeight += Weight;
        }
    }

    const float3 OutColor = (SumWeight > 0.0f) ? (SumColor / SumWeight) : HalfResColor.SampleLevel(PointSampler, TexCoord, 0).rgb;
    Output[Pixel] = float4(OutColor, 1.0f);
}
