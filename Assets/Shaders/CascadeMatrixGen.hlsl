#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Matrices.hlsli"

#define NUM_THREADS (4)

ConstantBuffer<FCamera>                CameraBuffer : register(b0);
ConstantBuffer<FCascadeGenerationInfo> Settings     : register(b1);

RWStructuredBuffer<FCascadeMatrices> MatrixBuffer : register(u0);
RWStructuredBuffer<FCascadeSplit>    SplitBuffer  : register(u1);

Texture2D<float2> MinMaxDepthTex : register(t0);

[numthreads(NUM_THREADS, 1, 1)]
void Main(FComputeShaderInput Input)
{
    const int CascadeIndex = int(Input.DispatchThreadID.x);
    
    // Get the min and max depth of the scene
    const float2 MinMaxDepth = MinMaxDepthTex[uint2(0, 0)];
    
    float4x4 InvCamera = CameraBuffer.ViewProjectionInverse;
    float NearPlane = CameraBuffer.NearPlane;
    float FarPlane  = CameraBuffer.FarPlane;
    float ClipRange = FarPlane - NearPlane;

    float MinDepth = NearPlane + ClipRange * saturate(MinMaxDepth.x);
    float MaxDepth = NearPlane + ClipRange * saturate(MinMaxDepth.y);
    
    float Temp = MinDepth;
    MinDepth = min(MinDepth, MaxDepth);
    MaxDepth = max(Temp, MaxDepth);

    float Range = MaxDepth - MinDepth;
    float Ratio = MaxDepth / MinDepth;

    float CascadeSplits[NUM_SHADOW_CASCADES];
    
    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        float Percentage = (i + 1) / float(NUM_SHADOW_CASCADES);
        float Log        = MinDepth * pow(abs(Ratio), Percentage);
        float Uniform    = MinDepth + Range * Percentage;
        float Distance   = Settings.CascadeSplitLambda * (Log - Uniform) + Uniform;
        CascadeSplits[i] = (Distance - NearPlane) / ClipRange;
    }

    const float CascadeResolution = Settings.CascadeResolution;

    float PrevSplitDist = (CascadeIndex == 0) ? MinMaxDepth.x : CascadeSplits[CascadeIndex - 1];
    float SplitDist     = CascadeSplits[CascadeIndex];

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
    {
        for (int j = 0; j < 8; j++)
        {
            float4 Corner = mul(float4(FrustumCorners[j], 1.0f), InvCamera);
            FrustumCorners[j] = Corner.xyz / Corner.w;
        }
    }

    {
        for (int j = 0; j < 4; j++)
        {
            const float3 Distance = FrustumCorners[j + 4] - FrustumCorners[j];
            FrustumCorners[j + 4] = FrustumCorners[j] + (Distance * SplitDist);
            FrustumCorners[j]     = FrustumCorners[j] + (Distance * PrevSplitDist);
        }
    }

    // Calc frustum center
    float3 FrustumCenter = Float3(0.0f);
    {
        for (int j = 0; j < 8; j++)
        {
            FrustumCenter += FrustumCorners[j];
        }
    }
    FrustumCenter /= 8.0f;

    float Radius = 0.0f;
    {
        for (int j = 0; j < 8; j++)
        {
            const float Distance = length(FrustumCorners[j] - FrustumCenter);
            Radius = max(Radius, Distance);
        }
    }
    Radius = ceil(Radius * 16.0f) / 16.0f;
    
    float3 MaxExtents = Float3(Radius);
    float3 MinExtents = -MaxExtents;
     
    float3 CascadeExtents = MaxExtents - MinExtents;
    float3 LightDirection = normalize(Settings.LightDirection);
    float3 ShadowEyePos   = FrustumCenter - (LightDirection * MaxExtents.z);
    
    const float3 LIGHT_UP = float3(0.0f, 0.0f, 1.0f);
    float3 LightRight = normalize(cross(LIGHT_UP, LightDirection));
    float3 LightUp    = normalize(cross(LightDirection, LightRight));
    
    float4x4 ViewMat = CreateLookToMatrix(ShadowEyePos, LightDirection, LightUp);

    // Add extra to capture more objects in the frustum
    const float ExtentZ      = CascadeExtents.z + 50.0f;
    const float NewNearPlane = -ExtentZ;
    const float NewFarPlane  = ExtentZ;
    float4x4 OrtoMat = CreateOrtographicProjection(
        MinExtents.x,
        MaxExtents.x,
        MinExtents.y,
        MaxExtents.y,
        NewNearPlane,
        NewFarPlane);
    
    // Stabilize cascades
    float4x4 ShadowMatrix = mul(ViewMat, OrtoMat);
    float3   ShadowOrigin = mul(float4(Float3(0.0f), 1.0f), ShadowMatrix).xyz;
    ShadowOrigin *= (CascadeResolution / 2.0f);
    
    float3 RoundedOrigin = round(ShadowOrigin);
    float3 RoundedOffset = RoundedOrigin - ShadowOrigin;
    RoundedOffset   = RoundedOffset * (2.0f / CascadeResolution);
    RoundedOffset.z = 0;

    float4x4 OrtoMatOrig = OrtoMat;
    OrtoMat[3][0] += RoundedOffset.x;
    OrtoMat[3][1] += RoundedOffset.y;
    
    // Create final matrices
    FCascadeMatrices Matrices;
    Matrices.View     = ViewMat;
    Matrices.ViewProj = mul(ViewMat, OrtoMat);
    
    MatrixBuffer[CascadeIndex] = Matrices;
    
    // Write splits
    FCascadeSplit Split;
    Split.MinExtent = MinExtents;
    Split.MaxExtent = MaxExtents;
    Split.Split     = NearPlane + SplitDist * ClipRange;
    Split.NearPlane = NewNearPlane;
    Split.FarPlane  = NewFarPlane;
   
    Split.Padding0   = 0.0f;
    Split.Padding1   = 0.0f;
    Split.Padding2   = 0.0f;

    SplitBuffer[CascadeIndex] = Split;
}