#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "Shadows/CascadeStructs.hlsli"

#define DEBUG_VIEW_NONE 0
#define DEBUG_VIEW_SHADOW_MASK 1
#define DEBUG_VIEW_GBUFFER_ALBEDO 2
#define DEBUG_VIEW_GBUFFER_NORMAL 3
#define DEBUG_VIEW_GBUFFER_MATERIAL 4
#define DEBUG_VIEW_GBUFFER_VELOCITY 5
#define DEBUG_VIEW_SSAO 6
#define DEBUG_VIEW_DEPTH 7
#define DEBUG_VIEW_SHADOW_CASCADES 8
#define DEBUG_VIEW_SHADOW_CASCADE_INDEX 9
#define DEBUG_VIEW_SHADOW_CASCADE_OVERLAY 10
#define DEBUG_VIEW_LIT 11
#define DEBUG_VIEW_RAY_TRACING_REFLECTIONS_RAW 12
#define DEBUG_VIEW_RAY_TRACING_REFLECTIONS_TEMPORAL 13
#define DEBUG_VIEW_RAY_TRACING_REFLECTIONS_SPATIAL 14
#define DEBUG_VIEW_RAY_TRACING_PRIMARY_ID 15

// Mirrors FSceneRenderView::EDebugViewChannel
#define DEBUG_VIEW_CHANNEL_RED   1
#define DEBUG_VIEW_CHANNEL_GREEN 2
#define DEBUG_VIEW_CHANNEL_BLUE  4
#define DEBUG_VIEW_CHANNEL_ALPHA 8

Texture2D<float4>     GBufferAlbedo        : register(t0);  // rgb = base color
Texture2D<float4>     GBufferNormal        : register(t1);  // rgb = world-space normal (packed)
Texture2D<float4>     GBufferMaterial      : register(t2);  // r   = AO, g = Roughness, b = Metallic
Texture2D<float4>     GBufferVelocity      : register(t3);  // rg  = NDC motion vector
Texture2D<float>      GBufferDepth         : register(t4);  // r   = device depth
Texture2D<float>      ShadowMask           : register(t5);  // r   = directional shadow factor
Texture2D<float>      SSAOBuffer           : register(t6);  // r   = ambient occlusion
Texture2DArray<float> ShadowCascades       : register(t7);  // r   = cascade shadow depth (per slice)
Texture2D<uint>       CascadeIndexBuffer   : register(t8);  // r   = selected cascade index
Texture2D<float4>     LitSceneBuffer       : register(t9);  // rgb = composited lit scene color
Texture2D<float4>     RayTracedReflections : register(t10); // rgb = final a-trous resolved reflection radiance
Texture2D<float4>     ReflectionTrace      : register(t11); // rgb = raw 1-spp traced reflection radiance, a = hit distance
Texture2D<float4>     ReflectionTemporal   : register(t12); // rgb = temporally-accumulated radiance (current ReflectionHistory)

SamplerState LinearSampler : register(s0);
SamplerState PointSampler  : register(s1);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    int DebugMode;
    int ShadowMapSize;
    int OutputWidth;
    int OutputHeight;

    // 16-32
    int ViewX;
    int ViewY;
    int TargetWidth;
    int TargetHeight;

    // 32-40
    int bIsOutputSceneTarget;
    int ChannelMask;
SHADER_CONSTANT_BLOCK_END

float3 ApplyChannelMask(float3 Color, float Alpha, int ChannelMask)
{
    const bool bRed   = (ChannelMask & DEBUG_VIEW_CHANNEL_RED)   != 0;
    const bool bGreen = (ChannelMask & DEBUG_VIEW_CHANNEL_GREEN) != 0;
    const bool bBlue  = (ChannelMask & DEBUG_VIEW_CHANNEL_BLUE)  != 0;
    const bool bAlpha = (ChannelMask & DEBUG_VIEW_CHANNEL_ALPHA) != 0;

    if (bAlpha && !bRed && !bGreen && !bBlue)
    {
        return Alpha.xxx;
    }

    float3 Result = float3(bRed ? Color.r : 0.0, bGreen ? Color.g : 0.0, bBlue ? Color.b : 0.0);
    if (bAlpha)
    {
        Result *= Alpha;
    }

    return Result;
}

float3 VisualizeDepth(float Depth)
{
    float LinearDepth = DepthToLinear(CameraBuffer.NearPlane, CameraBuffer.FarPlane, Depth);
    float Depth01     = saturate((LinearDepth - CameraBuffer.NearPlane) / max(CameraBuffer.FarPlane - CameraBuffer.NearPlane, 0.0001));
    
    return (1.0 - Depth01).xxx;
}

float3 CascadeIndexToColor(uint CascadeIndex)
{
    if (CascadeIndex == 0)
    {
        return float3(1.0, 0.0, 0.0);
    }

    if (CascadeIndex == 1)
    {
        return float3(0.0, 1.0, 0.0);
    }

    if (CascadeIndex == 2)
    {
        return float3(0.0, 0.0, 1.0);
    }

    if (CascadeIndex == 3)
    {
        return float3(1.0, 1.0, 0.0);
    }

    return float3(1.0, 1.0, 1.0);
}

float ComputeViewDepth(float2 TexCoord, float Depth)
{
    const float X = TexCoord.x * 2.0 - 1.0 - CameraBuffer.Jitter.x;
    const float Y = (1.0 - TexCoord.y) * 2.0 - 1.0 - CameraBuffer.Jitter.y;
    
    const float4 ProjectedPos = float4(X, Y, Depth, 1.0);
    const float4 WorldPos4    = mul(ProjectedPos, CameraBuffer.ViewProjectionInvUnjittered);
    const float  InvW         = (abs(WorldPos4.w) > 1e-6) ? rcp(WorldPos4.w) : 0.0;
    const float3 PositionWS   = WorldPos4.xyz * InvW;
    
    return max(dot(PositionWS - CameraBuffer.PositionWS, CameraBuffer.Forward), 0.0);
}

float4 Main(float2 TexCoord : TEXCOORD0) : SV_Target
{
    float3 Color = 0.0;
    float  Alpha = 1.0;

    const float2 FullTexCoord = TexCoord;
    if (Constants.DebugMode == DEBUG_VIEW_SHADOW_MASK)
    {
        const float Mask = ShadowMask.SampleLevel(PointSampler, FullTexCoord, 0).r;
        Color = Mask.xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_ALBEDO)
    {
        const float4 AlbedoSample = GBufferAlbedo.SampleLevel(LinearSampler, FullTexCoord, 0);
        Color = AlbedoSample.rgb;
        Alpha = AlbedoSample.a;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_NORMAL)
    {
        Color = GBufferNormal.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_MATERIAL)
    {
        Color = GBufferMaterial.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_VELOCITY)
    {
        float2 Vel = GBufferVelocity.SampleLevel(PointSampler, FullTexCoord, 0).rg;
        Color = float3(Vel * 0.5 + 0.5, 0.0);
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SSAO)
    {
        const float AO = SSAOBuffer.SampleLevel(PointSampler, FullTexCoord, 0).r;
        Color = AO.xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_DEPTH)
    {
        const float Depth = GBufferDepth.SampleLevel(PointSampler, FullTexCoord, 0).r;
        Color = VisualizeDepth(Depth);
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SHADOW_CASCADES)
    {
        const float2 ViewportSize = float2(Constants.OutputWidth, Constants.OutputHeight);
        const float2 Pixel        = TexCoord * ViewportSize;
        const float  MinDim       = min(ViewportSize.x, ViewportSize.y);
        const float  QuadSize     = floor(MinDim * 0.5);
        const float2 GridSize     = float2(QuadSize * 2.0, QuadSize * 2.0);
        const float2 GridMin      = 0.5 * (ViewportSize - GridSize);

        if (QuadSize < 1.0)
        {
            return float4(0.0, 0.0, 0.0, 1.0);
        }

        if (any(Pixel < GridMin) || any(Pixel >= (GridMin + GridSize)))
        {
            return float4(0.0, 0.0, 0.0, 1.0);
        }

        const float2 Local = Pixel - GridMin;

        const uint X = (Local.x >= QuadSize) ? 1 : 0;
        const uint Y = (Local.y >= QuadSize) ? 1 : 0;

        const uint CascadeIndex = Y * 2 + X;
        if (CascadeIndex >= NUM_SHADOW_CASCADES)
        {
            return float4(0.0, 0.0, 0.0, 1.0);
        }

        const float2 QuadPixel = Local - float2(float(X), float(Y)) * QuadSize;
        const float2 QuadUV    = (QuadPixel + 0.5) / QuadSize;
        const float  Depth     = ShadowCascades.SampleLevel(PointSampler, float3(QuadUV, CascadeIndex), 0).r;

        Color = (1.0 - Depth).xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SHADOW_CASCADE_INDEX)
    {
        const float2 FullSize     = float2(Constants.TargetWidth, Constants.TargetHeight);
        const uint2  Pixel        = (uint2)min(max(TexCoord * FullSize, 0.0), FullSize - 1.0);
        const uint   CascadeIndex = CascadeIndexBuffer.Load(int3(Pixel, 0));

        Color = CascadeIndexToColor(CascadeIndex);
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SHADOW_CASCADE_OVERLAY)
    {
        const float2 FullSize     = float2(Constants.TargetWidth, Constants.TargetHeight);
        const uint2  Pixel        = (uint2)min(max(TexCoord * FullSize, 0.0), FullSize - 1.0);
        const uint   CascadeIndex = CascadeIndexBuffer.Load(int3(Pixel, 0));
        const float3 OverlayColor = CascadeIndexToColor(CascadeIndex);
        const float  Depth        = GBufferDepth.SampleLevel(PointSampler, FullTexCoord, 0).r;
        const float3 LitColor     = LitSceneBuffer.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
        const float  Mask         = (Depth >= 0.9999) ? 0.0 : 1.0;

        Color = lerp(LitColor, OverlayColor, 0.55 * Mask);
    }
    else if (Constants.DebugMode == DEBUG_VIEW_LIT)
    {
        Color = LitSceneBuffer.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_RAY_TRACING_REFLECTIONS_RAW)
    {
        Color = ReflectionTrace.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_RAY_TRACING_REFLECTIONS_TEMPORAL)
    {
        Color = ReflectionTemporal.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_RAY_TRACING_REFLECTIONS_SPATIAL)
    {
        Color = RayTracedReflections.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_RAY_TRACING_PRIMARY_ID)
    {
        Color = RayTracedReflections.SampleLevel(PointSampler, FullTexCoord, 0).rgb;
    }
    else
    {
        Color = 0.0;
    }

    Color = ApplyChannelMask(Color, Alpha, Constants.ChannelMask);

    if (Constants.bIsOutputSceneTarget == 0)
    {
        Color = LinearToSRGB(Color);
    }

    return float4(Color, 1.0);
}
