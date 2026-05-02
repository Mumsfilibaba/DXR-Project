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

Texture2D<float4>     GBufferAlbedo      : register(t0);
Texture2D<float4>     GBufferNormal      : register(t1);
Texture2D<float4>     GBufferMaterial    : register(t2);
Texture2D<float4>     GBufferVelocity    : register(t3);
Texture2D<float>      GBufferDepth       : register(t4);
Texture2D<float>      ShadowMask         : register(t5);
Texture2D<float>      SSAOBuffer         : register(t6);
Texture2DArray<float> ShadowCascades     : register(t7);
Texture2D<uint>       CascadeIndexBuffer : register(t8);
Texture2D<float4>     LitSceneBuffer     : register(t9);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler  : register(s1);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    int DebugMode;
    int ShadowMapSize;
    int OutputWidth;
    int OutputHeight;
    int ViewX;
    int ViewY;
    int TargetWidth;
    int TargetHeight;
    int bIsOutputSceneTarget;
SHADER_CONSTANT_BLOCK_END

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

    const float2 FullTexCoord = TexCoord;
    if (Constants.DebugMode == DEBUG_VIEW_SHADOW_MASK)
    {
        const float Mask = ShadowMask.SampleLevel(PointSampler, FullTexCoord, 0).r;
        Color = Mask.xxx;
    }
    else if (Constants.DebugMode == DEBUG_VIEW_GBUFFER_ALBEDO)
    {
        Color = GBufferAlbedo.SampleLevel(LinearSampler, FullTexCoord, 0).rgb;
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
    else
    {
        Color = 0.0;
    }

    if (Constants.bIsOutputSceneTarget == 0)
    {
        Color = LinearToSRGB(Color);
    }

    return float4(Color, 1.0);
}
