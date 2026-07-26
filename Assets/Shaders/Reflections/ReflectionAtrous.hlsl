#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "DepthHelpers.hlsli"

Texture2D<float4> InColorVariance : register(t0); // rgb = color, a = variance
Texture2D<float4> GBufferNormal   : register(t1); // rgb = world-space normal (packed)
Texture2D<float>  GBufferDepth    : register(t2); // r   = device depth

SamplerState GBufferSampler : register(s0); // point + clamp

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> OutColorVariance : register(u0); // rgb = filtered color, a = variance

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    float2 ScreenSize;
    int    StepSize;
    float  PhiColor;
SHADER_CONSTANT_BLOCK_END

ConstantBuffer<FCamera> CameraBuffer : register(b0);

static const float KernelWeights[3] = { 3.0f / 8.0f, 1.0f / 4.0f, 1.0f / 16.0f };

[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const int2 Pixel = int2(DispatchThreadID.xy);
    if (Pixel.x >= int(Constants.ScreenSize.x) || Pixel.y >= int(Constants.ScreenSize.y))
    {
        return;
    }

    const float2 CenterCoord = (float2(Pixel) + 0.5f) / Constants.ScreenSize;
    const float  CenterDepth = GBufferDepth.SampleLevel(GBufferSampler, CenterCoord, 0);

    if (CenterDepth >= 1.0f)
    {
        OutColorVariance[Pixel] = InColorVariance[Pixel];
        return;
    }

    const float4 Center         = InColorVariance[Pixel];
    const float3 CenterColor    = Center.rgb;
    const float  CenterVariance = Center.a;
    const float  CenterLuma     = Luminance(CenterColor);
    const float3 CenterNormal   = UnpackNormal(GBufferNormal.SampleLevel(GBufferSampler, CenterCoord, 0).rgb);
    const float  LumaPhi        = Constants.PhiColor * sqrt(max(CenterVariance, 1e-8f)) + 1e-4f;

    float3 SumColor    = CenterColor * KernelWeights[0] * KernelWeights[0];
    float  SumVariance = CenterVariance * (KernelWeights[0] * KernelWeights[0]) * (KernelWeights[0] * KernelWeights[0]);
    float  SumWeight   = KernelWeights[0] * KernelWeights[0];

    [unroll]
    for (int dy = -2; dy <= 2; ++dy)
    {
        [unroll]
        for (int dx = -2; dx <= 2; ++dx)
        {
            if (dx == 0 && dy == 0)
            {
                continue;
            }

            const int2 TapPixel = Pixel + int2(dx, dy) * Constants.StepSize;
            if (any(TapPixel < 0) || TapPixel.x >= int(Constants.ScreenSize.x) || TapPixel.y >= int(Constants.ScreenSize.y))
            {
                continue;
            }

            const float2 TapCoord = (float2(TapPixel) + 0.5f) / Constants.ScreenSize;
            const float  TapDepth = GBufferDepth.SampleLevel(GBufferSampler, TapCoord, 0);

            if (TapDepth >= 1.0f)
            {
                continue;
            }

            const float4 Tap          = InColorVariance[TapPixel];
            const float3 TapColor     = Tap.rgb;
            const float3 TapNormal    = UnpackNormal(GBufferNormal.SampleLevel(GBufferSampler, TapCoord, 0).rgb);
            const float  WNormal      = pow(max(0.0f, dot(CenterNormal, TapNormal)), 64.0f);
            const float  WDepth       = exp(-abs(CenterDepth - TapDepth) / (0.01f));
            const float  WLuma        = exp(-abs(CenterLuma - Luminance(TapColor)) / LumaPhi);
            const float  KernelWeight = KernelWeights[abs(dx)] * KernelWeights[abs(dy)];
            const float  Weight       = KernelWeight * WNormal * WDepth * WLuma;

            SumColor    += TapColor * Weight;
            SumVariance += Tap.a * (Weight * Weight);
            SumWeight   += Weight;
        }
    }

    const float  InvWeight = (SumWeight > 0.0f) ? (1.0f / SumWeight) : 0.0f;
    const float3 OutColor  = SumColor * InvWeight;
    const float  OutVar    = SumVariance * (InvWeight * InvWeight);

    OutColorVariance[Pixel] = float4(OutColor, OutVar);
}
