#include "../Structs.hlsli"
#include "../Helpers.hlsli"

#define NUM_THREADS 16

Texture2D<float>  SourceShadow : register(t0);
Texture2D<float2> MomentsTex   : register(t1);
Texture2D<float>  DepthBuffer  : register(t2);
Texture2D<float3> NormalBuffer : register(t3);

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float> Output : register(u0);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    int   StepWidth;
    int   bCopyOnly;
    float DepthSigma;
    float NormalSigma;
    float VarianceBoost;
    float Padding0;
SHADER_CONSTANT_BLOCK_END

uint2 ClampPixel(int2 PixelCoord)
{
    const int2 ViewportSize = int2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    PixelCoord = clamp(PixelCoord, int2(0, 0), ViewportSize - 1);
    return uint2(PixelCoord);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 Pixel = DispatchThreadID.xy;
    if (Pixel.x >= uint(CameraBuffer.ViewportWidth) || Pixel.y >= uint(CameraBuffer.ViewportHeight))
    {
        return;
    }

    const float CenterShadow = SourceShadow.Load(int3(Pixel, 0));
    if (Constants.bCopyOnly != 0)
    {
        Output[Pixel] = CenterShadow;
        return;
    }

    const float CenterDepthRaw = DepthBuffer.Load(int3(Pixel, 0));
    if (CenterDepthRaw >= 1.0)
    {
        Output[Pixel] = CenterShadow;
        return;
    }

    const float3 CenterNormal = UnpackNormal(NormalBuffer.Load(int3(Pixel, 0)));
    const float2 CenterMoments = MomentsTex.Load(int3(Pixel, 0));
    const float Variance = max(CenterMoments.y - (CenterMoments.x * CenterMoments.x), 0.0);
    const float VarianceFactor = 1.0 + (sqrt(Variance) * max(Constants.VarianceBoost, 0.0));

    static const float Kernel[5] = { 1.0, 4.0, 6.0, 4.0, 1.0 };

    float WeightedSum = 0.0;
    float WeightSum   = 0.0;

    const int Step = max(Constants.StepWidth, 1);
    [loop]
    for (int Y = -2; Y <= 2; ++Y)
    {
        [loop]
        for (int X = -2; X <= 2; ++X)
        {
            const uint2 SamplePixel = ClampPixel(int2(Pixel) + int2(X * Step, Y * Step));

            const float SampleShadow = SourceShadow.Load(int3(SamplePixel, 0));
            const float SampleDepth  = DepthBuffer.Load(int3(SamplePixel, 0));
            const float3 SampleNormal = UnpackNormal(NormalBuffer.Load(int3(SamplePixel, 0)));

            const float SpatialWeight = Kernel[abs(X)] * Kernel[abs(Y)];

            const float DepthDiff = abs(SampleDepth - CenterDepthRaw);
            const float DepthWeight = exp(-DepthDiff * max(Constants.DepthSigma, 1.0) * VarianceFactor);

            const float NormalDot = saturate(dot(CenterNormal, SampleNormal));
            const float NormalWeight = pow(max(NormalDot, 1e-4), max(Constants.NormalSigma, 1.0));

            const float SignalWeight = rcp(1.0 + abs(SampleShadow - CenterShadow) * VarianceFactor * 8.0);

            const float Weight = SpatialWeight * DepthWeight * NormalWeight * SignalWeight;
            WeightedSum += SampleShadow * Weight;
            WeightSum   += Weight;
        }
    }

    Output[Pixel] = (WeightSum > 0.0) ? (WeightedSum / WeightSum) : CenterShadow;
}
