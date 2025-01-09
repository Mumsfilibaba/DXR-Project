#include "../Helpers.hlsli"
#include "../Structs.hlsli"
#include "../Constants.hlsli"
#include "../Matrices.hlsli"
#include "CascadeStructs.hlsli"

#define NUM_THREADS (4)

ConstantBuffer<FCamera>                CameraBuffer   : register(b0);
ConstantBuffer<FCascadeGenerationInfo> GenerationInfo : register(b1);

RWStructuredBuffer<FCascadeMatrices> MatrixBuffer : register(u0);
RWStructuredBuffer<FCascadeSplit>    SplitBuffer  : register(u1);

Texture2D<float2> MinMaxDepthTex : register(t0);

[numthreads(NUM_THREADS, 1, 1)]
void Main(FComputeShaderInput Input)
{
    // Retrieve the cascade-index for this thread
    const int MaxCascadeIndex = min(GenerationInfo.MaxCascadeIndex, NUM_THREADS);
    const int CascadeIndex    = min(int(Input.DispatchThreadID.x), MaxCascadeIndex);
    
    // Get the minimum and maximum depth of the scene
    float2 MinMaxDepth = float2(0.0, 1.0);
    if (GenerationInfo.bDepthReductionEnabled)
    {
        MinMaxDepth = saturate(MinMaxDepthTex[uint2(0, 0)]);
    }

    float CameraNearPlane = CameraBuffer.NearPlane;
    float CameraFarPlane  = CameraBuffer.FarPlane;
    float ClipRange       = CameraFarPlane - CameraNearPlane;

    float MinDepth = CameraNearPlane + ClipRange * MinMaxDepth.x;
    float MaxDepth = CameraNearPlane + ClipRange * MinMaxDepth.y;
    
    float Range = MaxDepth - MinDepth;
    float Ratio = MaxDepth / max(MinDepth, 0.01);

    float CascadeSplits[NUM_SHADOW_CASCADES];
    
    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    {
        const float SplitLambda = GenerationInfo.CascadeSplitLambda;

        [unroll]
        for (int Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            float Percentage     = (Index + 1) / float(NUM_SHADOW_CASCADES);
            float LogScale       = MinDepth * pow(abs(Ratio), Percentage);
            float UniformScale   = MinDepth + Range * Percentage;
            float Distance       = SplitLambda * (LogScale - UniformScale) + UniformScale;
            CascadeSplits[Index] = (Distance - CameraNearPlane) / ClipRange;
        }
    }

    // Use min between MinMaxDepth to protect against cases where the lowest is 1.0 and highest 0.0 
    // This can happen when nothing is rendered in the prepass.
    float SplitDist     = CascadeSplits[CascadeIndex];
    float PrevSplitDist = (CascadeIndex == 0) ? min(MinMaxDepth.x, MinMaxDepth.y) : CascadeSplits[CascadeIndex - 1];

    float3 FrustumCornersWS[8] =
    {
        float3(-1.0,  1.0, 0.0),
        float3( 1.0,  1.0, 0.0),
        float3( 1.0, -1.0, 0.0),
        float3(-1.0, -1.0, 0.0),
        float3(-1.0,  1.0, 1.0),
        float3( 1.0,  1.0, 1.0),
        float3( 1.0, -1.0, 1.0),
        float3(-1.0, -1.0, 1.0),
    };

    // Calculate position of light frustum
    {
        float4x4 ViewProjectionInv = CameraBuffer.ViewProjectionInvUnjittered;

        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            float4 Corner = mul(float4(FrustumCornersWS[Index], 1.0), ViewProjectionInv);
            FrustumCornersWS[Index] = Corner.xyz / Corner.w;
        }
    }

    {
        [unroll]
        for (int Index = 0; Index < 4; ++Index)
        {
            const float3 Distance = FrustumCornersWS[Index + 4] - FrustumCornersWS[Index];
            FrustumCornersWS[Index + 4] = FrustumCornersWS[Index] + (Distance * SplitDist);
            FrustumCornersWS[Index]     = FrustumCornersWS[Index] + (Distance * PrevSplitDist);
        }
    }

    // Calculate the center of this frustum view slice
    float3 FrustumCenter = 0.0;
    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            FrustumCenter += FrustumCornersWS[Index];
        }

        FrustumCenter /= 8.0;
    }

    // Calculate the radius of the sphere that currounds the frustum
    float SphereRadius = 0.0;
    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            const float Distance = length(FrustumCornersWS[Index] - FrustumCenter);
            SphereRadius = max(SphereRadius, Distance);
        }

        SphereRadius = ceil(SphereRadius);
    }

    float3 MaxExtents     =  SphereRadius;
    float3 MinExtents     = -MaxExtents;  
    float3 CascadeExtents =  MaxExtents - MinExtents;

    // We use a specific extent in the z-direction, this is in order to prevent that some
    // objects are not visibe in the shadow-map and that are "behind" the camera. 
    float LightNearPlane = 100.0;
    float LightFarPlane  = 300.0;

    // Setup ShadowView
    float3 LightDirection = normalize(GenerationInfo.LightDirection);
    float3 ShadowEyePos   = FrustumCenter - (LightDirection * 200.0);
    
    // Constant upvector in order to keep the cascades stable
    float3 LightUp = float3(0.0, 1.0, 0.0);
    
    float3x3 LightRotationMatrix;
    LightRotationMatrix[2] = LightDirection;
    LightRotationMatrix[0] = normalize(cross(LightUp, LightRotationMatrix[2]));
    LightRotationMatrix[1] = cross(LightRotationMatrix[2], LightRotationMatrix[0]);

    // Matrices
    float4x4 ShadowView       = InverseRotationTranslation(LightRotationMatrix, ShadowEyePos);
    float4x4 ShadowProjection = OrtographicMatrix(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, LightNearPlane, LightFarPlane);
    
    // Stabilize cascades
    {
        const float CascadeResolution = GenerationInfo.CascadeResolution;

        float4x4 ShadowMatrix = mul(ShadowView, ShadowProjection);
        float3   ShadowOrigin = 0.0;
        ShadowOrigin  = mul(float4(ShadowOrigin, 1.0), ShadowMatrix).xyz;
        ShadowOrigin *= (CascadeResolution / 2.0);
        
        float3 RoundedOrigin = round(ShadowOrigin);
        float3 RoundedOffset = RoundedOrigin - ShadowOrigin;
        RoundedOffset   = RoundedOffset * (2.0 / CascadeResolution);
        RoundedOffset.z = 0.0;

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
    float4x4 InvShadowView = float4x4(
        float4(LightRotationMatrix[0], 0.0),
        float4(LightRotationMatrix[1], 0.0),
        float4(LightRotationMatrix[2], 0.0),
        float4(ShadowEyePos, 1.0));

    float4x4 InvShadowProjection = InverseScaleTranslation(ShadowProjection);
    float4x4 InvShadowMatrix     = mul(InvShadowView, InvShadowProjection);
    
    float3 Corners[8] =
    {
        float3( 1.0, -1.0, 0.0),
        float3(-1.0, -1.0, 0.0),
        float3( 1.0,  1.0, 0.0),
        float3(-1.0,  1.0, 0.0),
        float3( 1.0, -1.0, 1.0),
        float3(-1.0, -1.0, 1.0),
        float3( 1.0,  1.0, 1.0),
        float3(-1.0,  1.0, 1.0),
    };

    {
        [unroll]
        for(int Index = 0; Index < 8; ++Index)
        {
            float4 Corner = mul(float4(Corners[Index], 1.0), InvShadowMatrix);
            Corners[Index] = Corner.xyz / Corner.w;
        }
    }

    float4 FrustumPlanes[NUM_FRUSTUM_PLANES];
    FrustumPlanes[0] = PlaneFromPoints(Corners[0], Corners[4], Corners[2]);
    FrustumPlanes[1] = PlaneFromPoints(Corners[1], Corners[3], Corners[5]);
    FrustumPlanes[2] = PlaneFromPoints(Corners[3], Corners[2], Corners[7]);
    FrustumPlanes[3] = PlaneFromPoints(Corners[1], Corners[5], Corners[0]);
    FrustumPlanes[4] = PlaneFromPoints(Corners[5], Corners[7], Corners[4]);
    FrustumPlanes[5] = PlaneFromPoints(Corners[1], Corners[0], Corners[3]);

    // Create a matrix that converts from [-1, 1] -> [0, 1]
    const float4x4 TextureScaleBias = float4x4(
        float4(0.5,  0.0, 0.0, 0.0),
        float4(0.0, -0.5, 0.0, 0.0),
        float4(0.0,  0.0, 1.0, 0.0),
        float4(0.5,  0.5, 0.0, 1.0));
        
    const float4x4 InvTextureScaleBias = InverseScaleTranslation(TextureScaleBias);
    const float4x4 InvCascadeMatrix    = mul(mul(InvTextureScaleBias, InvShadowProjection), InvShadowView);
    
    // Calculate the position of the lower corner of the cascade partition, in the UV space of the first cascade partition...
    float3 CascadeCorner = mul(float4(0.0, 0.0, 0.0, 1.0), InvCascadeMatrix).xyz;
    CascadeCorner = mul(float4(CascadeCorner, 1.0), GenerationInfo.ShadowMatrix).xyz;

    // ... and then the same for the upper window.
    float3 OtherCorner = mul(float4(1.0, 1.0, 1.0, 1.0), InvCascadeMatrix).xyz;
    OtherCorner = mul(float4(OtherCorner, 1.0), GenerationInfo.ShadowMatrix).xyz;
      
    // Store Split-Data
    {
        FCascadeSplit Split;
        Split.MinExtent     = MinExtents;
        Split.MaxExtent     = MaxExtents;
        Split.NearPlane     = LightNearPlane;
        Split.FarPlane      = LightFarPlane;
        Split.MinDepth      = MinDepth;
        Split.MaxDepth      = MaxDepth;
        Split.Split         = CameraNearPlane + SplitDist * ClipRange;
        Split.PreviousSplit = CameraNearPlane + PrevSplitDist * ClipRange;

        [unroll]
        for(int Index = 0; Index < NUM_FRUSTUM_PLANES; ++Index)
        {
            Split.FrustumPlanes[Index] = FrustumPlanes[Index];
        }

        float3 CascadeScale = 1.0 / (OtherCorner - CascadeCorner);
        Split.Offsets = float4(-CascadeCorner, 0.0);
        Split.Scale   = float4( CascadeScale, 1.0);

        SplitBuffer[CascadeIndex] = Split;
    }
}