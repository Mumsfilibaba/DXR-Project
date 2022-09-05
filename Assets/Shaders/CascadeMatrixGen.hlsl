#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Matrices.hlsli"

#define NUM_THREADS (4)

ConstantBuffer<FCamera>                CameraBuffer   : register(b0);
ConstantBuffer<FCascadeGenerationInfo> GenerationInfo : register(b1);

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
    {
        [unroll]
        for (int Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            float Percentage = (Index + 1) / float(NUM_SHADOW_CASCADES);
            float Log        = MinDepth * pow(abs(Ratio), Percentage);
            float Uniform    = MinDepth + Range * Percentage;
            float Distance   = GenerationInfo.CascadeSplitLambda * (Log - Uniform) + Uniform;
            CascadeSplits[Index] = (Distance - NearPlane) / ClipRange;
        }
    }

    const float CascadeResolution = GenerationInfo.CascadeResolution;

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
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            float4 Corner = mul(float4(FrustumCorners[Index], 1.0f), InvCamera);
            FrustumCorners[Index] = Corner.xyz / Corner.w;
        }
    }

    {
        [unroll]
        for (int Index = 0; Index < 4; ++Index)
        {
            const float3 Distance = FrustumCorners[Index + 4] - FrustumCorners[Index];
            FrustumCorners[Index + 4] = FrustumCorners[Index] + (Distance * SplitDist);
            FrustumCorners[Index]     = FrustumCorners[Index] + (Distance * PrevSplitDist);
        }
    }

    // Calc frustum center
    float3 FrustumCenter = Float3(0.0f);
    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            FrustumCenter += FrustumCorners[Index];
        }
    }
    FrustumCenter /= 8.0f;

    float Radius = 0.0f;
    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            const float Distance = length(FrustumCorners[Index] - FrustumCenter);
            Radius = max(Radius, Distance);
        }
    }
    Radius = ceil(Radius * 16.0f) / 16.0f;
    
    float3 MaxExtents = Float3(Radius);
    float3 MinExtents = -MaxExtents;
     
    // Setup ShadowView
    float3 CascadeExtents = MaxExtents - MinExtents;
    float3 LightDirection = normalize(GenerationInfo.LightDirection);
    float3 ShadowEyePos   = FrustumCenter - (LightDirection * MaxExtents.z);
    
    // Constant upvector in order to keep the cascades stable
    float3 LightUp = float3(0.0f, 1.0f, 0.0f);
    
    float3x3 LightRotationMatrix;
    LightRotationMatrix[2] = LightDirection;
    LightRotationMatrix[0] = normalize(cross(LightUp, LightRotationMatrix[2]));
    LightRotationMatrix[1] = cross(LightRotationMatrix[2], LightRotationMatrix[0]);

    float4x4 ShadowView = InverseRotationTranslation(LightRotationMatrix, ShadowEyePos);

    // Add extra to capture more objects in the frustum
    const float ExtentZ      = CascadeExtents.z + 50.0f;
    const float NewNearPlane = -ExtentZ;
    const float NewFarPlane  =  ExtentZ;
    
    // Projection Matrix
    float4x4 ShadowProjection = OrtographicMatrix(
        MinExtents.x,
        MaxExtents.x,
        MinExtents.y,
        MaxExtents.y,
        NewNearPlane,
        NewFarPlane);
    
    // Stabilize cascades
    {
        float4x4 ShadowMatrix = mul(ShadowView, ShadowProjection);
        float3   ShadowOrigin = Float3(0.0f);
        ShadowOrigin = mul(float4(ShadowOrigin, 1.0f), ShadowMatrix).xyz;
        ShadowOrigin *= (CascadeResolution / 2.0f);
        
        float3 RoundedOrigin = round(ShadowOrigin);
        float3 RoundedOffset = RoundedOrigin - ShadowOrigin;
        RoundedOffset   = RoundedOffset * (2.0f / CascadeResolution);
        RoundedOffset.z = 0.0f;

        ShadowProjection[3][0] += RoundedOffset.x;
        ShadowProjection[3][1] += RoundedOffset.y;
    }
    
    // Create final matrix
    float4x4 ShadowMatrix = mul(ShadowView, ShadowProjection);
    
    // Store final matrices
    {
        FCascadeMatrices Matrices;
        Matrices.View     = ShadowView;
        Matrices.ViewProj = ShadowMatrix;
        MatrixBuffer[CascadeIndex] = Matrices;
    }
    
    // Create Frustom Planes
    float4x4 InverseShadowView = float4x4(
        float4(LightRotationMatrix[0], 0.0f),
        float4(LightRotationMatrix[1], 0.0f),
        float4(LightRotationMatrix[2], 0.0f),
        float4(ShadowEyePos          , 1.0f));

    float4x4 InverseShadowProjection = InverseScaleTranslation(ShadowProjection);
    float4x4 InverseShadowMatrix     = mul(InverseShadowView, InverseShadowProjection);
    
    float3 Corners[8] =
    {
        float3( 1.0f, -1.0f, 0.0f),
        float3(-1.0f, -1.0f, 0.0f),
        float3( 1.0f,  1.0f, 0.0f),
        float3(-1.0f,  1.0f, 0.0f),
        float3( 1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
        float3( 1.0f,  1.0f, 1.0f),
        float3(-1.0f,  1.0f, 1.0f),
    };

    {
        [unroll]
        for(int Index = 0; Index < 8; ++Index)
        {
            float4 Corner = mul(float4(Corners[Index], 1.0f), InverseShadowMatrix);
            Corners[Index] = Corner.xyz / Corner.w;
        }
    }

    float4 FrustumPlanes[6];
    FrustumPlanes[0] = PlaneFromPoints(Corners[0], Corners[4], Corners[2]);
    FrustumPlanes[1] = PlaneFromPoints(Corners[1], Corners[3], Corners[5]);
    FrustumPlanes[2] = PlaneFromPoints(Corners[3], Corners[2], Corners[7]);
    FrustumPlanes[3] = PlaneFromPoints(Corners[1], Corners[5], Corners[0]);
    FrustumPlanes[4] = PlaneFromPoints(Corners[5], Corners[7], Corners[4]);
    FrustumPlanes[5] = PlaneFromPoints(Corners[1], Corners[0], Corners[3]);

    // Create a matrix that converts from [-1, 1] -> [0, 1]
    const float4x4 TextureScaleBias = float4x4(
        float4(0.5f,  0.0f, 0.0f, 0.0f),
        float4(0.0f, -0.5f, 0.0f, 0.0f),
        float4(0.0f,  0.0f, 1.0f, 0.0f),
        float4(0.5f,  0.5f, 0.0f, 1.0f));
    const float4x4 InverseTextureScaleBias = InverseScaleTranslation(TextureScaleBias);

    // Calculate the position of the lower corner of the cascade partition, in the UV space
    // of the first cascade partition
    const float4x4 InverseCascadeMatrix = mul(mul(InverseTextureScaleBias, InverseShadowProjection), InverseShadowView);
    
    float3 CascadeCorner = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), InverseCascadeMatrix).xyz;
    CascadeCorner        = mul(float4(CascadeCorner, 1.0f), GenerationInfo.ShadowMatrix).xyz;

    float3 OtherCorner = mul(float4(1.0f, 1.0f, 1.0f, 1.0f), InverseCascadeMatrix).xyz;
    OtherCorner        = mul(float4(OtherCorner, 1.0f), GenerationInfo.ShadowMatrix).xyz;
      
    // Store Split-Data
    {
        FCascadeSplit Split;
        Split.MinExtent = MinExtents;
        Split.MaxExtent = MaxExtents;
        Split.Split     = NearPlane + SplitDist * ClipRange;
        Split.NearPlane = NewNearPlane;
        Split.FarPlane  = NewFarPlane;
    
        Split.Padding0  = 0.0f;
        Split.Padding1  = 0.0f;
        Split.Padding2  = 0.0f;

        [unroll]
        for(int Index = 0; Index < 6; ++Index)
        {
            Split.FrustumPlanes[Index + CascadeIndex * 6] = FrustumPlanes[Index];
        }

        float3 CascadeScale = 1.0f / (OtherCorner - CascadeCorner);
        Split.Offsets = float4(-CascadeCorner, 0.0f);
        Split.Scale   = float4( CascadeScale, 1.0f);

        SplitBuffer[CascadeIndex] = Split;
    }
}