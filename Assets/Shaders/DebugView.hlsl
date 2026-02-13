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

Texture2D<float4> GBufferAlbedo   : register(t0);
Texture2D<float4> GBufferNormal   : register(t1);
Texture2D<float4> GBufferMaterial : register(t2);
Texture2D<float4> GBufferVelocity : register(t3);
Texture2D<float>  GBufferDepth    : register(t4);
Texture2D<float>  ShadowMask      : register(t5);
Texture2D<float>  SSAOBuffer      : register(t6);
Texture2DArray<float> ShadowCascades : register(t7);
Texture2D<uint>   CascadeIndexBuffer : register(t8);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler  : register(s1);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    int DebugMode;
    int Padding0;
    int Padding1;
    int Padding2;
SHADER_CONSTANT_BLOCK_END

float3 VisualizeDepth(float Depth)
{
    float ViewZ = Depth_ProjToView(Depth, CameraBuffer.ProjectionInv);
    float Depth01 = saturate(ViewZ / max(CameraBuffer.FarPlane, 0.0001));
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
        const float2 QuadUV = frac(TexCoord * 2.0);
        const uint X = (TexCoord.x >= 0.5) ? 1 : 0;
        const uint Y = (TexCoord.y >= 0.5) ? 1 : 0;
        const uint CascadeIndex = Y * 2 + X;

        const float Depth = ShadowCascades.SampleLevel(PointSampler, float3(QuadUV, CascadeIndex), 0).r;
        Color = (1.0 - Depth).xxx;
    }
    else
    {
        Color = 0.0;
    }

    return float4(Color, 1.0);
}
