#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Matrix.hlsli"
#include "CascadeStructs.hlsli"

#define NUM_THREADS (NUM_SHADOW_CASCADES)

ConstantBuffer<FCamera>                CameraBuffer   : register(b0);
ConstantBuffer<FCascadeGenerationInfo> GenerationInfo : register(b1);

RWStructuredBuffer<FCascadeMatrices> MatrixBuffer   : register(u0);
RWStructuredBuffer<FCascadeSplit>    SplitBuffer    : register(u1);
Texture2D<float2>                    MinMaxDepthTex : register(t0);

[numthreads(NUM_THREADS, 1, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    // Retrieve the cascade-index for this thread
    const int CascadeIndex = min(int(DispatchThreadID.x), min(GenerationInfo.MaxCascadeIndex, NUM_SHADOW_CASCADES - 1));
    
    // Get the minimum and maximum depth of the scene
    float2 MinMaxDepth = float2(0.0, 1.0);

    [branch]
    if (GenerationInfo.bEnableTightFrustum)
    {
        MinMaxDepth = saturate(MinMaxDepthTex[uint2(0, 0)]);
    }

    float NearPlane = CameraBuffer.NearPlane;
    float ClipRange = CameraBuffer.FarPlane - NearPlane;

    float MinDepth = NearPlane + ClipRange * MinMaxDepth.x;
    float MaxDepth = NearPlane + ClipRange * MinMaxDepth.y;
    
    float CascadeSplits[NUM_SHADOW_CASCADES];
    
    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    {
        const float Range = MaxDepth - MinDepth;
        const float Ratio = MaxDepth / max(MinDepth, 0.01);

        [unroll]
        for (int Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            float Percentage   = (Index + 1) / float(NUM_SHADOW_CASCADES);
            float LogScale     = MinDepth * pow(abs(Ratio), Percentage);
            float UniformScale = MinDepth + Range * Percentage;
            float Distance     = GenerationInfo.CascadeSplitLambda * (LogScale - UniformScale) + UniformScale;

            CascadeSplits[Index] = (Distance - NearPlane) / ClipRange;
        }
    }

    // Calculate position of light frustum in world-space
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

    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            float4 Corner = mul(float4(FrustumCornersWS[Index], 1.0), CameraBuffer.ViewProjectionInvUnjittered);
            FrustumCornersWS[Index] = Corner.xyz / Corner.w;
        }
    }

    // Use min between MinMaxDepth to protect against cases where the lowest is 1.0 
    // and highest 0.0. This can happen when nothing is rendered in the prepass.
    float SplitDist     = CascadeSplits[CascadeIndex];
    float PrevSplitDist = (CascadeIndex == 0) ? min(MinMaxDepth.x, MinMaxDepth.y) : CascadeSplits[CascadeIndex - 1];

    {
        [unroll]
        for (int Index = 0; Index < 4; ++Index)
        {
            float3 CornerRay     = FrustumCornersWS[Index + 4] - FrustumCornersWS[Index];
            float3 NearCornerRay = CornerRay * PrevSplitDist;
            float3 FarCornerRay  = CornerRay * SplitDist;

            FrustumCornersWS[Index + 4] = FrustumCornersWS[Index] + FarCornerRay;
            FrustumCornersWS[Index]     = FrustumCornersWS[Index] + NearCornerRay;
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

        SphereRadius = ceil(SphereRadius * 16.0) / 16.0;
    }

    // Cache the shadow-map size
    const float CascadeResolution = GenerationInfo.CascadeResolution;

    // Calculate the extents for this cascade...
    float3 MaxExtents     =  SphereRadius;
    float3 MinExtents     = -MaxExtents;
    float3 CascadeExtents =  MaxExtents - MinExtents;

    // We use a specific extent in the z-direction, this is in order to prevent that some
    // objects are not visibe in the shadow-map and that are "behind" the camera.
#if 1
    float LightNearPlane      = GenerationInfo.LightNearPlane;
    float LightFarPlane       = GenerationInfo.LightFarPlane;
    float LightPositionOffset = GenerationInfo.LightPositionOffset;
#else
    float LightNearPlane      = 0.0;
    float LightFarPlane       = CascadeExtents.z;
    float LightPositionOffset = -MinExtents.z;
#endif

    // Setup Shadow-View
    float3 LightDirection = normalize(GenerationInfo.LightDirection);
    
    // Create the position for the shadow rendering
    float3 ShadowEyePos = FrustumCenter - LightDirection * LightPositionOffset;

    // Constant upvector in order to keep the cascades stable
    float3 LightUp = float3(0.0, 1.0, 0.0);
    
    // Create the view matrix and it's inverse
    float3x3 LightRotation;
    LightRotation[2] = LightDirection;
    LightRotation[0] = normalize(cross(LightUp, LightRotation[2]));
    LightRotation[1] = cross(LightRotation[2], LightRotation[0]);

    float4x4 View    = Matrix::InvRotationTranslation(LightRotation, ShadowEyePos);
    float4x4 InvView = float4x4(float4(LightRotation[0], 0.0), float4(LightRotation[1], 0.0), float4(LightRotation[2], 0.0), float4(ShadowEyePos, 1.0));

    // Create the projection
    float4x4 Projection = Matrix::OrthographicProjection(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, LightNearPlane, LightFarPlane);
    
    // Stabilize cascades
    [branch]
    if (GenerationInfo.bEnableStableCascades)
    {
        // Create a temportary view-projection matrix used to stabilize the cascades
        float4x4 ShadowViewProj = mul(View, Projection);

        const float ShadowTexelSize = 2.0 / CascadeResolution;

        float3 ShadowOrigin = mul(float4(0.0, 0.0, 0.0, 1.0), ShadowViewProj).xyz;
        ShadowOrigin = ShadowOrigin * CascadeResolution * 0.5;

        float3 RoundedOrigin = floor(ShadowOrigin);
        float3 RoundedOffset = (RoundedOrigin - ShadowOrigin) * ShadowTexelSize;

        Projection[3][0] += RoundedOffset.x;
        Projection[3][1] += RoundedOffset.y;
    }

    // Create the final view-projection matrix after we have stabilized the projection matrix
    float4x4 ViewProjection = mul(View, Projection);

    // Create inverse matrices
    float4x4 InvProjection     = Matrix::InvScaleTranslation(Projection);
    float4x4 InvViewProjection = mul(InvView, InvProjection);

    // Store final matrices
    {
        FCascadeMatrices Matrices;
        PackMatrix(View,              Matrices.View);
        PackMatrix(ViewProjection,    Matrices.ViewProj);
        PackMatrix(InvView,           Matrices.InvView);
        PackMatrix(InvViewProjection, Matrices.InvViewProj);

        MatrixBuffer[CascadeIndex] = Matrices;
    }

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
            float4 Corner = mul(float4(Corners[Index], 1.0), InvViewProjection);
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
        
    const float4x4 InvTextureScaleBias = Matrix::InvScaleTranslation(TextureScaleBias);
    const float4x4 InvCascadeMatrix    = mul(mul(InvTextureScaleBias, InvProjection), InvView);
    
    // Calculate the position of the lower corner of the cascade partition, in the UV space of the first cascade partition...
    float3 LowerCorner = mul(float4(0.0, 0.0, 0.0, 1.0), InvCascadeMatrix).xyz;
    LowerCorner = mul(float4(LowerCorner, 1.0), GenerationInfo.ShadowMatrix).xyz;

    // ... and then the same for the upper window.
    float3 UpperCorner = mul(float4(1.0, 1.0, 1.0, 1.0), InvCascadeMatrix).xyz;
    UpperCorner = mul(float4(UpperCorner, 1.0), GenerationInfo.ShadowMatrix).xyz;
    
    // Store Split-Data
    {
        FCascadeSplit Split;
        Split.MinExtent     = MinExtents;
        Split.MaxExtent     = MaxExtents;
        Split.NearPlane     = LightNearPlane;
        Split.FarPlane      = LightFarPlane;
        Split.MinDepth      = MinDepth;
        Split.MaxDepth      = MaxDepth;
        Split.Split         = NearPlane + SplitDist * ClipRange;
        Split.PreviousSplit = NearPlane + PrevSplitDist * ClipRange;

        [unroll]
        for(int Index = 0; Index < NUM_FRUSTUM_PLANES; ++Index)
        {
            Split.FrustumPlanes[Index] = FrustumPlanes[Index];
        }

        // Scales used when we sample the cascades later
        float3 CascadeScale = 1.0 / (UpperCorner - LowerCorner);
        Split.Offsets = float4(-LowerCorner, 0.0);
        Split.Scale   = float4(CascadeScale, 1.0);
        
        // We might want the position for the cascade
        Split.CascadeCameraPosition = ShadowEyePos;
        Split.Padding0              = SphereRadius;

        SplitBuffer[CascadeIndex] = Split;
    }
}