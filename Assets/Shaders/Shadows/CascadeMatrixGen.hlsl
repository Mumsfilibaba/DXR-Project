#include "../Helpers.hlsli"
#include "../Structs.hlsli"
#include "../Constants.hlsli"
#include "../Matrix.hlsli"
#include "CascadeStructs.hlsli"

#define NUM_THREADS (NUM_SHADOW_CASCADES)

ConstantBuffer<FCamera> CameraBuffer : register(b0);
ConstantBuffer<FCascadeGenerationInfo> GenerationInfo : register(b1);

RWStructuredBuffer<FCascadeMatrices> MatrixBuffer : register(u0);
RWStructuredBuffer<FCascadeSplit> SplitBuffer : register(u1);
RWTexture2D<float2> MinMaxDepthHistory : register(u2);

Texture2D<float2> MinMaxDepthTex : register(t0);

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
        if (MinMaxDepth.x > MinMaxDepth.y)
        {
            MinMaxDepth = float2(0.0, 1.0);
        }

        // Stabilize against sub-pixel jitter / small rasterization changes:
        // - Quantize min depth down and max depth up (keeps the range conservative).
        const float DepthQuant = max(GenerationInfo.TightFrustumDepthQuant, 1.0);
        MinMaxDepth.x = saturate(floor(MinMaxDepth.x * DepthQuant) / DepthQuant);
        MinMaxDepth.y = saturate(ceil(MinMaxDepth.y * DepthQuant) / DepthQuant);

        // Add a small conservative padding to reduce temporal "breathing" and prevent
        // extremely tight ranges that can cause out-of-frustum sampling artifacts.
        const float DepthPad = 2.0 / DepthQuant;
        MinMaxDepth.x = saturate(MinMaxDepth.x - DepthPad);
        MinMaxDepth.y = saturate(MinMaxDepth.y + DepthPad);

        // Apply hysteresis to reduce temporal popping: expand immediately, shrink slowly.
        float2 PrevMinMax = MinMaxDepthHistory[uint2(0, 0)];
        if (PrevMinMax.x < 0.0 || PrevMinMax.y < 0.0 || PrevMinMax.x > PrevMinMax.y)
        {
            PrevMinMax = MinMaxDepth;
        }

        const float ShrinkFactor = saturate(GenerationInfo.TightFrustumShrinkFactor);
        float2 SmoothedMinMax;
        SmoothedMinMax.x = (MinMaxDepth.x < PrevMinMax.x) ? MinMaxDepth.x : lerp(PrevMinMax.x, MinMaxDepth.x, ShrinkFactor);
        SmoothedMinMax.y = (MinMaxDepth.y > PrevMinMax.y) ? MinMaxDepth.y : lerp(PrevMinMax.y, MinMaxDepth.y, ShrinkFactor);
        MinMaxDepth = SmoothedMinMax;

        if (DispatchThreadID.x == 0)
        {
            MinMaxDepthHistory[uint2(0, 0)] = MinMaxDepth;
        }
    }

    float NearPlane = CameraBuffer.NearPlane;
    float ClipRange = CameraBuffer.FarPlane - NearPlane;

    float MinDepth = NearPlane + ClipRange * MinMaxDepth.x;
    float MaxDepth = NearPlane + ClipRange * MinMaxDepth.y;
    
    float CascadeSplits[NUM_SHADOW_CASCADES];
    float CascadeSplitsReference[NUM_SHADOW_CASCADES];
    
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

    // Also calculate a reference set of splits using the full camera clip range (independent of tight-frustum min/max depth).
    // This is used for stable "texel -> world" conversions for filtering limits so that enabling tight frustum does not
    // change the perceived penumbra size as shadow-map density increases.
    {
        const float ReferenceMinDepth = NearPlane;
        const float ReferenceMaxDepth = CameraBuffer.FarPlane;

        const float Range = ReferenceMaxDepth - ReferenceMinDepth;
        const float Ratio = ReferenceMaxDepth / max(ReferenceMinDepth, 0.01);

        [unroll]
        for (int Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            float Percentage   = (Index + 1) / float(NUM_SHADOW_CASCADES);
            float LogScale     = ReferenceMinDepth * pow(abs(Ratio), Percentage);
            float UniformScale = ReferenceMinDepth + Range * Percentage;
            float Distance     = GenerationInfo.CascadeSplitLambda * (LogScale - UniformScale) + UniformScale;

            CascadeSplitsReference[Index] = (Distance - NearPlane) / ClipRange;
        }
    }

    // Calculate position of light frustum in world-space
    float3 FullFrustumCornersWS[8] =
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
            float4 Corner = mul(float4(FullFrustumCornersWS[Index], 1.0), CameraBuffer.ViewProjectionInvUnjittered);
            FullFrustumCornersWS[Index] = Corner.xyz / Corner.w;
        }
    }

    float3 FrustumCornersWS[8];
    float3 ReferenceFrustumCornersWS[8];

    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            FrustumCornersWS[Index]          = FullFrustumCornersWS[Index];
            ReferenceFrustumCornersWS[Index] = FullFrustumCornersWS[Index];
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

    // Slice the reference frustum corners using the full clip-range splits (independent of tight depth range).
    const float ReferenceSplitDist     = CascadeSplitsReference[CascadeIndex];
    const float ReferencePrevSplitDist = (CascadeIndex == 0) ? 0.0 : CascadeSplitsReference[CascadeIndex - 1];

    {
        [unroll]
        for (int Index = 0; Index < 4; ++Index)
        {
            float3 CornerRay     = ReferenceFrustumCornersWS[Index + 4] - ReferenceFrustumCornersWS[Index];
            float3 NearCornerRay = CornerRay * ReferencePrevSplitDist;
            float3 FarCornerRay  = CornerRay * ReferenceSplitDist;

            ReferenceFrustumCornersWS[Index + 4] = ReferenceFrustumCornersWS[Index] + FarCornerRay;
            ReferenceFrustumCornersWS[Index]     = ReferenceFrustumCornersWS[Index] + NearCornerRay;
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

    float3 ReferenceFrustumCenter = 0.0;

    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            ReferenceFrustumCenter += ReferenceFrustumCornersWS[Index];
        }

        ReferenceFrustumCenter /= 8.0;
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

    float ReferenceSphereRadius = 0.0;

    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            const float Distance = length(ReferenceFrustumCornersWS[Index] - ReferenceFrustumCenter);
            ReferenceSphereRadius = max(ReferenceSphereRadius, Distance);
        }

        ReferenceSphereRadius = ceil(ReferenceSphereRadius * 16.0) / 16.0;
    }

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

    float4x4 View    = FMatrix::InvRotationTranslation(LightRotation, ShadowEyePos);
    float4x4 InvView = float4x4(float4(LightRotation[0], 0.0), float4(LightRotation[1], 0.0), float4(LightRotation[2], 0.0), float4(ShadowEyePos, 1.0));

    // Cache the shadow-map size
    const float CascadeResolution = GenerationInfo.CascadeResolution;
    const float ReferenceWorldTexelSize = (2.0 * ReferenceSphereRadius) / max(CascadeResolution, 1.0);

    const bool bUseTightAABB = (GenerationInfo.bEnableTightFrustum != 0) && (GenerationInfo.TightFrustumForceSphereFit <= 0.5);
    const bool bStableExtents = (GenerationInfo.bEnableStableCascades != 0) && (GenerationInfo.TightFrustumStableExtents > 0.5);

    float3 MinExtents;
    float3 MaxExtents;

    if (bUseTightAABB)
    {
        float3 LSMin = float3(1e9, 1e9, 1e9);
        float3 LSMax = float3(-1e9, -1e9, -1e9);

        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            float3 CornerLS = mul(float4(FrustumCornersWS[Index], 1.0), View).xyz;
            LSMin = min(LSMin, CornerLS);
            LSMax = max(LSMax, CornerLS);
        }

        float3 RefLSMin = LSMin;
        float3 RefLSMax = LSMax;

        if (bStableExtents)
        {
            RefLSMin = float3(1e9, 1e9, 1e9);
            RefLSMax = float3(-1e9, -1e9, -1e9);

            [unroll]
            for (int Index = 0; Index < 8; ++Index)
            {
                float3 CornerLS = mul(float4(ReferenceFrustumCornersWS[Index], 1.0), View).xyz;
                RefLSMin = min(RefLSMin, CornerLS);
                RefLSMax = max(RefLSMax, CornerLS);
            }
        }

        const float2 MinXY = bStableExtents ? RefLSMin.xy : LSMin.xy;
        const float2 MaxXY = bStableExtents ? RefLSMax.xy : LSMax.xy;

        MinExtents = float3(MinXY, LSMin.z);
        MaxExtents = float3(MaxXY, LSMax.z);
    }
    else
    {
        float StableRadius = SphereRadius;
        if (GenerationInfo.bEnableTightFrustum && bStableExtents)
        {
            StableRadius = ReferenceSphereRadius;
        }

        MaxExtents = float3(StableRadius, StableRadius, SphereRadius);
        MinExtents = float3(-StableRadius, -StableRadius, -SphereRadius);
    }

    float3 CascadeExtents = MaxExtents - MinExtents;

    // Expand tight frustum to account for the maximum PCSS kernel footprint (guard band).
    {
        float MaxKernelWorld = max(GenerationInfo.MaxPenumbraWorld, GenerationInfo.MaxSearchDistanceWorld);
        if (GenerationInfo.bEnableTightFrustum && MaxKernelWorld > 0.0)
        {
            // Prevent guard bands from exploding the cascade when very large kernels are requested.
            const float MaxAbsX = max(abs(MinExtents.x), abs(MaxExtents.x));
            const float MaxAbsY = max(abs(MinExtents.y), abs(MaxExtents.y));
            const float MaxGuardBand = max(MaxAbsX, MaxAbsY) * 0.5;
            MaxKernelWorld = min(MaxKernelWorld, MaxGuardBand);
            MaxExtents.xy += MaxKernelWorld;
            MinExtents.xy -= MaxKernelWorld;
            CascadeExtents = MaxExtents - MinExtents;
        }
    }

    // Create the projection
    float4x4 Projection = FMatrix::OrthographicProjection(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, LightNearPlane, LightFarPlane);
    
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
    float4x4 InvProjection     = FMatrix::InvScaleTranslation(Projection);
    float4x4 InvViewProjection = mul(InvView, InvProjection);

    // Store final matrices
    {
        FCascadeMatrices Matrices;
        Matrices.View        = View;
        Matrices.ViewProj    = ViewProjection;
        Matrices.InvView     = InvView;
        Matrices.InvViewProj = InvViewProjection;

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
        
    const float4x4 InvTextureScaleBias = FMatrix::InvScaleTranslation(TextureScaleBias);
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
        Split.Padding0              = ReferenceWorldTexelSize;

        SplitBuffer[CascadeIndex] = Split;
    }
}
