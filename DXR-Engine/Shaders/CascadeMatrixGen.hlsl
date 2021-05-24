#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Matrices.hlsli"

#define NUM_THREADS 4

ConstantBuffer<Camera>             CameraBuffer : register(b0);
ConstantBuffer<SCascadeGenerationInfo> Settings : register(b1);

RWStructuredBuffer<SCascadeMatrices> MatrixBuffer : register(u0);

Texture2D<float2> MinMaxDepthTex : register(t0);

[numthreads(NUM_THREADS, 1, 1)]
void Main(ComputeShaderInput Input)
{
    const uint CascadeIndex = Input.DispatchThreadID.x;
    
    // Get the min and max depth of the scene
    float2 MinMaxDepth = MinMaxDepthTex[uint2(0, 0)];
    
    float4x4 InvCamera = CameraBuffer.ViewProjectionInverse;
    float NearPlane = CameraBuffer.NearPlane;
    float FarPlane  = CameraBuffer.FarPlane;
    float ClipRange = FarPlane - NearPlane;

    float MinDepth = NearPlane + ClipRange * MinMaxDepth.x;
    float MaxDepth = NearPlane + ClipRange * MinMaxDepth.y;

    float RealRange = MaxDepth - MinDepth;
    float Ratio = MaxDepth / MinDepth;

    float CascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    
    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint i = 0; i < 4; i++)
    {
        float p = (i + 1) / float(NUM_SHADOW_CASCADES);
        float Log = MinDepth * pow(Ratio, p);
        float Uniform = MinDepth + RealRange * p;
        float d = Settings.CascadeSplitLambda * (Log - Uniform) + Uniform;
        CascadeSplits[i] = (d - NearPlane) / ClipRange;
    }

    // TODO: This should be moved to the settings
    const float CascadeSizes[NUM_SHADOW_CASCADES] =
    {
        2048.0f, 2048.0f, 2048.0f, 4096.0f
    };

    float LastSplitDist = (CascadeIndex == 0) ? MinMaxDepth.x : CascadeSplits[CascadeIndex - 1];
    float SplitDist = CascadeSplits[CascadeIndex];

    float3 FrustumCorners[8] =
    {
        float3(-1.0f,  1.0f, 0.0f),
        float3( 1.0f,  1.0f, 0.0f),
        float3( 1.0f, -1.0f, 0.0f),
        float3(-1.0f, -1.0f, 0.0f),
        float3(-1.0f,  1.0f, 1.0f),
        float3( 1.0f,  1.0f, 1.0f),
        float3( 1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
    };

    // Calculate position of light frustum
    for (int j = 0; j < 8; j++)
    {
        float4 Corner = mul(float4(FrustumCorners[j], 1.0f), InvCamera);
        FrustumCorners[j] = Corner.xyz / Corner.w;
    }

    for (int j = 0; j < 4; j++)
    {
        const float3 Distance = FrustumCorners[j + 4] - FrustumCorners[j];
        FrustumCorners[j + 4] = FrustumCorners[j] + (Distance * SplitDist);
        FrustumCorners[j] = FrustumCorners[j] + (Distance * LastSplitDist);
    }

    // Calc frustum center
    float3 FrustumCenter = Float3(0.0f);
    for (int j = 0; j < 8; j++)
    {
        FrustumCenter += FrustumCorners[j];
    }
    FrustumCenter = FrustumCenter * (1.0f / 8.0f);

    float Radius = 0.0f;
    for (int j = 0; j < 8; j++)
    {
        const float Distance = ceil(length(FrustumCorners[j].xyz - FrustumCenter));
        Radius = max(Radius, Distance);
    }

    Radius = ceil(Radius * 16.0f) / 16.0f;
    
    float3 MaxExtents = Float3(Radius);
    float3 MinExtents = -MaxExtents;

    float3 LightDirection = normalize(Settings.LightDirection);
    float3 CascadeExtents = MaxExtents - MinExtents;
    float3 ShadowEyePos = FrustumCenter + LightDirection * -MinExtents.z;
    
    const float3 WORLD_UP = float3(0.0, 1.0f, 0.0f);
    float4x4 ViewMat = CreateLookToMatrix(ShadowEyePos, LightDirection, WORLD_UP);
    float4x4 OrtoMat = CreateOrtographicProjection(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, 0.0f, CascadeExtents.z);
    
    float4x4 ShadowMatrix = mul(ViewMat, OrtoMat);
    float3   ShadowOrigin = mul(float4(Float3(0.0f), 1.0f), ShadowMatrix);
    ShadowOrigin = ShadowOrigin * (CascadeSizes[CascadeIndex] / 2.0f);
    
    float3 RoundedOrigin = round(ShadowOrigin);
    float3 RoundedOffset = RoundedOrigin - ShadowOrigin;
    RoundedOffset = RoundedOffset * (2.0f / CascadeSizes[CascadeIndex]);

    OrtoMat[3][0] += RoundedOffset.x;
    OrtoMat[3][1] += RoundedOffset.y;
    
    // Create final matrices
    SCascadeMatrices Matrices;
    Matrices.View     = ViewMat;
    Matrices.ViewProj = mul(ViewMat, OrtoMat);
    
    MatrixBuffer[CascadeIndex] = Matrices;
}