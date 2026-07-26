#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "DepthHelpers.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0);

Texture2D<float4> ReflectionTrace    : register(t0); // rgb = radiance, a = ray hit distance (<0 on miss / background)
Texture2D<float4> GBufferNormal      : register(t1); // rgb = world-space normal (packed)
Texture2D<float>  GBufferDepth       : register(t2); // r   = device depth
Texture2D<float2> GBufferVelocity    : register(t3); // rg  = NDC motion vector
Texture2D<float4> HistoryColorPrev   : register(t4); // rgb = accumulated color, a = history length
Texture2D<float2> HistoryMomentsPrev : register(t5); // r   = luma mean, g = luma^2 mean

SamplerState LinearSampler : register(s0);

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> HistoryColorOut   : register(u0); // rgb = color, a = history length
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float2> HistoryMomentsOut : register(u1); // r   = luma mean, g = luma^2 mean
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> DenoisedOut       : register(u2); // rgb = color, a = variance (a-trous input)

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    float2 ScreenSize;
    float  TemporalAlpha;
    float  MaxRadiance;            // Per-sample luminance clamp (fireflies); <= 0 disables the clamp

    // 16-32
    float  HistoryClampGamma;      // Neighborhood clip width in std-devs; <= 0 disables history rectification
    float  MaxHistoryLength;       // Accumulation cap (frames)
    int    NeighborhoodRadius;     // Half-window (texels) for the neighborhood color box
    float  CameraMotionMaxHistory; // Alpha-floor accumulation cap while the camera is moving
SHADER_CONSTANT_BLOCK_END

float3 SanitizeColor(float3 Color)
{
    return (all(Color >= 0.0f) && all(Color < 1e20f)) ? Color : float3(0.0f, 0.0f, 0.0f);
}

float2 SanitizeMoments(float2 Moments)
{
    return (all(Moments >= 0.0f) && all(Moments < 1e20f)) ? Moments : float2(0.0f, 0.0f);
}

[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 Pixel = DispatchThreadID.xy;
    if (Pixel.x >= uint(Constants.ScreenSize.x) || Pixel.y >= uint(Constants.ScreenSize.y))
    {
        return;
    }

    const float2 TexCoord = (float2(Pixel) + 0.5f) / Constants.ScreenSize;
    const float4 Trace    = ReflectionTrace[Pixel];

    float3 CurrentColor = SanitizeColor(Trace.rgb);
    if (Constants.MaxRadiance > 0.0f)
    {
        const float CurrentLumaRaw = Luminance(CurrentColor);
        if (CurrentLumaRaw > Constants.MaxRadiance)
        {
            CurrentColor *= (Constants.MaxRadiance / max(CurrentLumaRaw, 1e-4f));
        }
    }

    const float Depth = GBufferDepth.SampleLevel(LinearSampler, TexCoord, 0);
    if (Depth >= 1.0f)
    {
        HistoryColorOut[Pixel]   = float4(0.0f, 0.0f, 0.0f, 0.0f);
        HistoryMomentsOut[Pixel] = float2(0.0f, 0.0f);
        DenoisedOut[Pixel]       = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    const float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    const float  HitT          = (Trace.a > 0.0f && Trace.a < 1e20f) ? Trace.a : -1.0f;

    float2 PrevTexCoord;
    if (HitT > 0.0f)
    {
        const float3 ViewDir     = normalize(WorldPosition - CameraBuffer.PositionWS);
        const float3 VirtualPos  = WorldPosition + ViewDir * HitT;
        const float4 PrevClip    = mul(float4(VirtualPos, 1.0f), CameraBuffer.PrevViewProjection);
        PrevTexCoord = (PrevClip.xy / PrevClip.w) * float2(0.5f, -0.5f) + 0.5f;
    }
    else
    {
        const float2 Velocity = GBufferVelocity.SampleLevel(LinearSampler, TexCoord, 0);
        PrevTexCoord = TexCoord - (Velocity * float2(0.5f, -0.5f));
    }

    float3 HistoryColor   = float3(0.0f, 0.0f, 0.0f);
    float2 HistoryMoments = float2(0.0f, 0.0f);
    float  HistoryLength  = 0.0f;
    bool   bHasHistory    = false;

    const bool bValidReproj = all(PrevTexCoord >= 0.0f) && all(PrevTexCoord <= 1.0f);
    if (bValidReproj)
    {
        const float4 SampledColor   = HistoryColorPrev.SampleLevel(LinearSampler, PrevTexCoord, 0);
        const float2 SampledMoments = HistoryMomentsPrev.SampleLevel(LinearSampler, PrevTexCoord, 0);
        const float3 SafeColor      = SanitizeColor(SampledColor.rgb);
        const float2 SafeMoments    = SanitizeMoments(SampledMoments);
        const float  SafeLength     = (SampledColor.a >= 0.0f && SampledColor.a < 1e20f) ? SampledColor.a : 0.0f;

        if (SafeLength > 0.0f)
        {
            HistoryColor   = SafeColor;
            HistoryMoments = SafeMoments;
            HistoryLength  = SafeLength;
            bHasHistory    = true;
        }
    }

    const float  CurrentLuma    = Luminance(CurrentColor);
    const float2 CurrentMoments = float2(CurrentLuma, CurrentLuma * CurrentLuma);
    const float3 CameraDelta    = CameraBuffer.PositionWS - CameraBuffer.PrevPositionWS;
    const bool   bCameraMoved   = dot(CameraDelta, CameraDelta) > 1e-8f;

    float3 AccumulatedColor;
    float2 AccMoments;

    if (bHasHistory)
    {
        if (Constants.HistoryClampGamma > 0.0f && Constants.NeighborhoodRadius > 0)
        {
            const int2 Size   = int2(Constants.ScreenSize);
            const int  Radius = Constants.NeighborhoodRadius;

            float3 M1    = float3(0.0f, 0.0f, 0.0f);
            float3 M2    = float3(0.0f, 0.0f, 0.0f);
            float  Count = 0.0f;

            for (int dy = -Radius; dy <= Radius; ++dy)
            {
                for (int dx = -Radius; dx <= Radius; ++dx)
                {
                    const int2   Tap      = clamp(int2(Pixel) + int2(dx, dy), int2(0, 0), Size - 1);
                    const float3 TapColor = SanitizeColor(ReflectionTrace[Tap].rgb);

                    M1    += TapColor;
                    M2    += TapColor * TapColor;
                    Count += 1.0f;
                }
            }

            const float3 Mean   = M1 / Count;
            const float3 StdDev = sqrt(max(float3(0.0f, 0.0f, 0.0f), (M2 / Count) - (Mean * Mean)));
            const float3 BoxMin = Mean - StdDev * Constants.HistoryClampGamma;
            const float3 BoxMax = Mean + StdDev * Constants.HistoryClampGamma;

            HistoryColor = ClipAABB(BoxMin, BoxMax, HistoryColor);
        }

        HistoryLength = min(HistoryLength + 1.0f, Constants.MaxHistoryLength);

        float MaxAccum = HistoryLength;
        if (bCameraMoved && Constants.CameraMotionMaxHistory > 0.0f)
        {
            MaxAccum = min(MaxAccum, Constants.CameraMotionMaxHistory);
        }

        const float Alpha = max(Constants.TemporalAlpha, 1.0f / MaxAccum);
        AccumulatedColor  = lerp(HistoryColor, CurrentColor, Alpha);
        AccMoments        = lerp(HistoryMoments, CurrentMoments, Alpha);
    }
    else
    {
        HistoryLength    = 1.0f;
        AccumulatedColor = CurrentColor;
        AccMoments       = CurrentMoments;
    }

    const float Variance = max(0.0f, AccMoments.y - (AccMoments.x * AccMoments.x));

    HistoryColorOut[Pixel]   = float4(AccumulatedColor, HistoryLength);
    HistoryMomentsOut[Pixel] = AccMoments;
    DenoisedOut[Pixel]       = float4(AccumulatedColor, Variance);
}
