#include "Structs.hlsli"
#include "Helpers.hlsli"

// Debug view modes (must match FSceneRenderView::EDebugView)
#define DEBUG_VIEW_NONE              0
#define DEBUG_VIEW_SHADOW_MASK       1
#define DEBUG_VIEW_GBUFFER_ALBEDO    2
#define DEBUG_VIEW_GBUFFER_NORMAL    3
#define DEBUG_VIEW_GBUFFER_MATERIAL  4
#define DEBUG_VIEW_GBUFFER_VELOCITY  5
#define DEBUG_VIEW_SSAO              6
#define DEBUG_VIEW_DEPTH             7
#define DEBUG_VIEW_SHADOW_CASCADES   8
#define DEBUG_VIEW_SHADOW_CASCADE_INDEX      9
#define DEBUG_VIEW_SHADOW_CASCADE_TRANSITION 10
#define DEBUG_VIEW_SHADOW_FILTER_MARGIN      11
#define DEBUG_VIEW_SHADOW_PCSS_RADIUS_CLAMP  12
#define DEBUG_VIEW_SHADOW_CASCADE_UPDATED    13
#define DEBUG_VIEW_SHADOW_CONTAINMENT        14
#define DEBUG_VIEW_SHADOW_CASCADE_FALLBACK   15
#define DEBUG_VIEW_SHADOW_CASCADE_OVERLAY    16

Texture2D<float4> GBufferAlbedo   : register(t0);
Texture2D<float4> GBufferNormal   : register(t1);
Texture2D<float4> GBufferMaterial : register(t2);
Texture2D<float4> GBufferVelocity : register(t3);
Texture2D<float>  GBufferDepth    : register(t4);
Texture2D<float>  ShadowMask      : register(t5);
Texture2D<float>  SSAOBuffer      : register(t6);
Texture2DArray<float> ShadowCascades : register(t7);
Texture2D<uint>   CascadeIndexBuffer : register(t8);
Texture2D<float4> ShadowDebugBuffer : register(t9);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler  : register(s1);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    int DebugMode;
    int ShadowMapSize;
    int OutputWidth;
    int OutputHeight;
SHADER_CONSTANT_BLOCK_END

float3 VisualizeDepth(float Depth)
{
    float LinearDepth = DepthToLinear(CameraBuffer.NearPlane, CameraBuffer.FarPlane, Depth);
    float Depth01 = saturate((LinearDepth - CameraBuffer.NearPlane) / max(CameraBuffer.FarPlane - CameraBuffer.NearPlane, 0.0001));
    return (1.0 - Depth01).xxx;
}

float4 Main(float2 TexCoord : TEXCOORD0) : SV_Target
{
    float3 Color = 0.0;

    if (Constants.DebugMode == DEBUG_VIEW_SHADOW_MASK)
    {
        const float Mask = ShadowMask.SampleLevel(PointSampler, TexCoord, 0).r;
        Color = Mask.xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_ALBEDO)
    {
        Color = GBufferAlbedo.SampleLevel(LinearSampler, TexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_NORMAL)
    {
        Color = GBufferNormal.SampleLevel(LinearSampler, TexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_MATERIAL)
    {
        Color = GBufferMaterial.SampleLevel(LinearSampler, TexCoord, 0).rgb;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_VELOCITY)
    {
        float2 Vel = GBufferVelocity.SampleLevel(PointSampler, TexCoord, 0).rg;
        Color = float3(Vel * 0.5 + 0.5, 0.0);
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SSAO)
    {
        const float AO = SSAOBuffer.SampleLevel(PointSampler, TexCoord, 0).r;
        Color = AO.xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_DEPTH)
    {
        const float Depth = GBufferDepth.SampleLevel(PointSampler, TexCoord, 0).r;
        Color = VisualizeDepth(Depth);
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SHADOW_CASCADES)
    {
        float2 ViewportSize = float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

        const float2 Pixel = TexCoord * ViewportSize;
        const float MinDim = min(ViewportSize.x, ViewportSize.y);
        const float QuadSize = floor(MinDim * 0.5);
        const float2 GridSize = float2(QuadSize * 2.0, QuadSize * 2.0);
        const float2 GridMin = 0.5 * (ViewportSize - GridSize);

        if (QuadSize < 1.0)
        {
            return float4(0.0, 0.0, 0.0, 1.0);
        }

        // Outside the square grid -> black
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
        const float2 QuadUV = (QuadPixel + 0.5) / QuadSize;

        const float Depth = ShadowCascades.SampleLevel(PointSampler, float3(QuadUV, CascadeIndex), 0).r;
        Color = (1.0 - Depth).xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_SHADOW_CASCADE_INDEX)
    {
        const float DebugValue = ShadowDebugBuffer.SampleLevel(PointSampler, TexCoord, 0).r;
        const float CascadeF = saturate(DebugValue) * max(float(NUM_SHADOW_CASCADES - 1), 1.0);
        const uint CascadeIndex = (uint)(CascadeF + 0.5);

        if (CascadeIndex == 0)
        {
            Color = float3(1.0, 0.0, 0.0);
        }
        else if (CascadeIndex == 1)
        {
            Color = float3(0.0, 1.0, 0.0);
        }
        else if (CascadeIndex == 2)
        {
            Color = float3(0.0, 0.0, 1.0);
        }
        else if (CascadeIndex == 3)
        {
            Color = float3(1.0, 1.0, 0.0);
        }
        else
        {
            Color = 1.0;
        }
    }
    else if (Constants.DebugMode >= DEBUG_VIEW_SHADOW_CASCADE_TRANSITION && Constants.DebugMode <= DEBUG_VIEW_SHADOW_CASCADE_FALLBACK)
    {
        const float3 DebugValue = ShadowDebugBuffer.SampleLevel(PointSampler, TexCoord, 0).rgb;
        Color = DebugValue;
    }
    else
    {
        Color = 0.0;
    }

    return float4(Color, 1.0);
}
