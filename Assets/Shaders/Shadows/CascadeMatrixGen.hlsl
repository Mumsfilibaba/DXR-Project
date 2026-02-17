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
RWStructuredBuffer<float4> CascadeSnapHistory : register(u3);
RWStructuredBuffer<float4> CascadeExtentsHistory : register(u4);

Texture2D<float2> MinMaxDepthTex : register(t0);

groupshared float2 gMinMaxDepth;

[numthreads(NUM_THREADS, 1, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    // Retrieve the cascade-index for this thread
    const int CascadeIndex = min(int(DispatchThreadID.x), min(GenerationInfo.MaxCascadeIndex, NUM_SHADOW_CASCADES - 1));
    
    // Get the minimum and maximum depth of the scene
    float2 MinMaxDepth = float2(0.0, 1.0);

    const bool bUseMinMaxDepth = (GenerationInfo.bEnableTightFrustum != 0) || (GenerationInfo.AdaptiveSplitRangeEnabled > 0.5);
    [branch]
    if (bUseMinMaxDepth)
    {
        if (DispatchThreadID.x == 0)
        {
            float2 LocalMinMax = saturate(MinMaxDepthTex[uint2(0, 0)]);
            if (LocalMinMax.x > LocalMinMax.y)
            {
                LocalMinMax = float2(0.0, 1.0);
            }

            // Stabilize against sub-pixel jitter / small rasterization changes:
            // - Quantize min depth down and max depth up (keeps the range conservative).
            const float DepthQuant = max(GenerationInfo.TightFrustumDepthQuant, 1.0);
            LocalMinMax.x = saturate(floor(LocalMinMax.x * DepthQuant) / DepthQuant);
            LocalMinMax.y = saturate(ceil(LocalMinMax.y * DepthQuant) / DepthQuant);

            // Add a small conservative padding to reduce temporal "breathing" and prevent
            // extremely tight ranges that can cause out-of-frustum sampling artifacts.
            const float DepthPad = 2.0 / DepthQuant;
            LocalMinMax.x = saturate(LocalMinMax.x - DepthPad);
            LocalMinMax.y = saturate(LocalMinMax.y + DepthPad);

            // Apply hysteresis to reduce temporal popping: expand immediately, shrink slowly.
            float2 PrevMinMax = MinMaxDepthHistory[uint2(0, 0)];
            if (PrevMinMax.x < 0.0 || PrevMinMax.y < 0.0 || PrevMinMax.x > PrevMinMax.y)
            {
                PrevMinMax = LocalMinMax;
            }

            const float ShrinkFactor = saturate(GenerationInfo.TightFrustumShrinkFactor);
            float2 SmoothedMinMax;
            SmoothedMinMax.x = (LocalMinMax.x < PrevMinMax.x) ? LocalMinMax.x : lerp(PrevMinMax.x, LocalMinMax.x, ShrinkFactor);
            SmoothedMinMax.y = (LocalMinMax.y > PrevMinMax.y) ? LocalMinMax.y : lerp(PrevMinMax.y, LocalMinMax.y, ShrinkFactor);
            gMinMaxDepth = SmoothedMinMax;
        }

        GroupMemoryBarrierWithGroupSync();
        MinMaxDepth = gMinMaxDepth;
        GroupMemoryBarrierWithGroupSync();

        if (DispatchThreadID.x == 0)
        {
            MinMaxDepthHistory[uint2(0, 0)] = MinMaxDepth;
        }
    }

    float CameraNear = CameraBuffer.NearPlane;
    float CameraFar  = CameraBuffer.FarPlane;
    const float MaxShadowDistance = GenerationInfo.MaxShadowDistance;
    if (MaxShadowDistance > CameraNear && MaxShadowDistance < CameraFar)
    {
        CameraFar = MaxShadowDistance;
    }

    const float CameraRange   = max(CameraFar - CameraNear, 1e-6);
    const float ClipRange     = max(CameraFar - CameraNear, 1e-6);
    const float NearPlane     = CameraNear;

    float MinDepth = CameraNear + CameraRange * MinMaxDepth.x;
    float MaxDepth = CameraNear + CameraRange * MinMaxDepth.y;
    MinDepth = clamp(MinDepth, CameraNear, CameraFar);
    MaxDepth = clamp(MaxDepth, MinDepth, CameraFar);
    
    const bool bAdaptiveSplitRange = (GenerationInfo.AdaptiveSplitRangeEnabled > 0.5);
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

    // Also calculate a reference set of splits.
    // When adaptive split range is enabled, use the adaptive min/max to keep reference coverage tight.
    // Otherwise, use the full camera clip range for stable penumbra behavior.
    {
        const float ReferenceMinDepth = bAdaptiveSplitRange ? MinDepth : NearPlane;
        const float ReferenceMaxDepth = bAdaptiveSplitRange ? MaxDepth : CameraFar;

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
    float3 FullFrustumCornersWS[8];

    {
        // Build world-space frustum corners using the clamped CameraFar (MaxShadowDistance).
        // This ensures max shadow distance actually affects cascade coverage.
        const float2 CornerXY[4] =
        {
            float2(-1.0,  1.0),
            float2( 1.0,  1.0),
            float2( 1.0, -1.0),
            float2(-1.0, -1.0),
        };

        [unroll]
        for (int Index = 0; Index < 4; ++Index)
        {
            const float2 XY = CornerXY[Index];
            float4 CornerVS = mul(float4(XY.x, XY.y, 1.0, 1.0), CameraBuffer.ProjectionInvUnjittered);
            CornerVS.xyz /= max(CornerVS.w, 1e-6);

            const float3 DirVS = normalize(CornerVS.xyz);
            const float3 NearVS = DirVS * CameraNear;
            const float3 FarVS  = DirVS * CameraFar;

            float4 NearWS = mul(float4(NearVS, 1.0), CameraBuffer.ViewInv);
            float4 FarWS  = mul(float4(FarVS, 1.0), CameraBuffer.ViewInv);

            FullFrustumCornersWS[Index]     = NearWS.xyz / max(NearWS.w, 1e-6);
            FullFrustumCornersWS[Index + 4] = FarWS.xyz / max(FarWS.w, 1e-6);
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
    const float AdaptiveNear = (MinDepth - NearPlane) / max(ClipRange, 1e-6);
    float SplitDist     = bAdaptiveSplitRange ? CascadeSplits[CascadeIndex] : CascadeSplitsReference[CascadeIndex];
    float PrevSplitDist = (CascadeIndex == 0) ? (bAdaptiveSplitRange ? AdaptiveNear : 0.0) : (bAdaptiveSplitRange ? CascadeSplits[CascadeIndex - 1] : CascadeSplitsReference[CascadeIndex - 1]);

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

    // Setup Shadow-View
    float3 LightDirection = normalize(GenerationInfo.LightDirection);
    
    // Create the position for the shadow rendering
    // Optional offset along the light direction (positive pushes the eye "upstream").
    float3 ShadowEyePos = FrustumCenter - LightDirection * GenerationInfo.LightPositionOffset;

    // Robust up-vector selection to avoid degeneracy near world-up
    const float3 WorldUp = float3(0.0, 1.0, 0.0);
    const float3 AltUp   = float3(0.0, 0.0, 1.0);
    const float  UpDot   = abs(dot(LightDirection, WorldUp));
    float3 LightUp = (UpDot > 0.99) ? AltUp : WorldUp;
    
    // Create the view matrix and it's inverse
    float3x3 LightRotation;
    LightRotation[2] = LightDirection;
    LightRotation[0] = normalize(cross(LightUp, LightRotation[2]));
    LightRotation[1] = cross(LightRotation[2], LightRotation[0]);

    float4x4 View    = Matrix::InvRotationTranslation(LightRotation, ShadowEyePos);
    float4x4 InvView = float4x4(float4(LightRotation[0], 0.0), float4(LightRotation[1], 0.0), float4(LightRotation[2], 0.0), float4(ShadowEyePos, 1.0));

    // Cache the shadow-map size
    const float CascadeResolution = GenerationInfo.CascadeResolution;
    const float ReferenceWorldTexelSize = (2.0 * ReferenceSphereRadius) / max(CascadeResolution, 1.0);

    const bool bUseTightAABB = (GenerationInfo.bEnableTightFrustum != 0) && (GenerationInfo.CascadeFitAABB > 0.5);
    const bool bStableExtents = (GenerationInfo.bEnableStableCascades != 0) && (GenerationInfo.TightFrustumStableExtents > 0.5);

    float3 MinExtents;
    float3 MaxExtents;

    float3 SliceLSMin = float3(1e9, 1e9, 1e9);
    float3 SliceLSMax = float3(-1e9, -1e9, -1e9);

    {
        [unroll]
        for (int Index = 0; Index < 8; ++Index)
        {
            float3 CornerLS = mul(float4(FrustumCornersWS[Index], 1.0), View).xyz;
            SliceLSMin = min(SliceLSMin, CornerLS);
            SliceLSMax = max(SliceLSMax, CornerLS);
        }
    }

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

    // Apply hysteresis to tight-frustum XY extents: expand immediately, shrink slowly.
    if (GenerationInfo.bEnableTightFrustum && bUseTightAABB)
    {
        float4 PrevExtents = CascadeExtentsHistory[CascadeIndex];
        const bool bPrevValid = (PrevExtents.z > PrevExtents.x) && (PrevExtents.w > PrevExtents.y);
        if (!bPrevValid)
        {
            PrevExtents = float4(MinExtents.xy, MaxExtents.xy);
        }

        const float ShrinkFactor = saturate(GenerationInfo.TightFrustumShrinkFactor);
        float2 NewMin = lerp(PrevExtents.xy, MinExtents.xy, ShrinkFactor);
        float2 NewMax = lerp(PrevExtents.zw, MaxExtents.xy, ShrinkFactor);

        // Expand immediately (component-wise), shrink slowly.
        NewMin = min(NewMin, MinExtents.xy);
        NewMax = max(NewMax, MaxExtents.xy);

        MinExtents.xy = NewMin;
        MaxExtents.xy = NewMax;

        CascadeExtentsHistory[CascadeIndex] = float4(NewMin, NewMax);
    }

    float3 CascadeExtents = MaxExtents - MinExtents;

    // Derived PCF/PCSS margins (in texels)
    const float RefTexelSize = max(ReferenceWorldTexelSize, 1e-6);
    const float PCFRadiusTexels = max(GenerationInfo.PCFMinFilterRadiusTexels, (GenerationInfo.PCFFilterWorld / RefTexelSize) * 0.5);
    const float PCFMarginTexels = ceil(PCFRadiusTexels) + 1.0;

    const float BasePCSS = CascadeResolution / 32.0;
    const uint CascadeShift = (1u << CascadeIndex);
    const float CascadeScale = rcp(float(CascadeShift));
    const float MaxPCSSRadiusTexels = clamp(BasePCSS * CascadeScale, 4.0, 64.0);
    const float PCSSMarginTexels = MaxPCSSRadiusTexels + 3.0;

    const bool bUsePCSS = (GenerationInfo.FilterMode != 0);
    const float SelectedMarginTexels = bUsePCSS ? PCSSMarginTexels : PCFMarginTexels;
    float MarginWorld = SelectedMarginTexels * RefTexelSize;

    // Expand extents to account for kernel footprint (guard band).
    if (MarginWorld > 0.0)
    {
        // Prevent guard bands from exploding the cascade when very large kernels are requested.
        const float MaxAbsX = max(abs(MinExtents.x), abs(MaxExtents.x));
        const float MaxAbsY = max(abs(MinExtents.y), abs(MaxExtents.y));
        const float MaxGuardBand = max(MaxAbsX, MaxAbsY) * 0.5;
        MarginWorld = min(MarginWorld, MaxGuardBand);
        MaxExtents.xy += MarginWorld;
        MinExtents.xy -= MarginWorld;
        CascadeExtents = MaxExtents - MinExtents;
    }

    float CascadeUpdatedThisFrame = 1.0;

    // Stabilize cascades by snapping extents in light-space
    [branch]
    if (GenerationInfo.bEnableStableCascades)
    {
        const float2 ExtentXY = MaxExtents.xy - MinExtents.xy;
        const float2 TexelSizeLS = ExtentXY / max(CascadeResolution, 1.0);
        const float2 Center = 0.5 * (MinExtents.xy + MaxExtents.xy);
        const float2 CenterSnapped = floor(Center / TexelSizeLS) * TexelSizeLS;

        const float2 PrevCenter = CascadeSnapHistory[CascadeIndex].xy;
        const bool bPrevValid = all(PrevCenter > -1e8);
        if (bPrevValid)
        {
            const float2 Delta = abs(CenterSnapped - PrevCenter);
            const float2 Threshold = TexelSizeLS * 0.5;
            CascadeUpdatedThisFrame = any(Delta > Threshold) ? 1.0 : 0.0;
        }

        CascadeSnapHistory[CascadeIndex] = float4(CenterSnapped, 0.0, 0.0);

        MinExtents.xy = CenterSnapped - 0.5 * ExtentXY;
        MaxExtents.xy = CenterSnapped + 0.5 * ExtentXY;
    }

    const float SliceDepth = max(SliceLSMax.z - SliceLSMin.z, 0.0);
    const bool bShadowPancaking = (GenerationInfo.ShadowPancakingEnabled > 0.5);
    float ZPadding = 0.0;
    if (bShadowPancaking)
    {
        // Tighter padding to improve depth precision when pancaking is enabled.
        const float MinPad = max(ReferenceWorldTexelSize * 2.0, 0.01);
        ZPadding = max(max(MinPad, SliceDepth * 0.01), 0.01);
    }
    else
    {
        ZPadding = max(max(ReferenceWorldTexelSize * 16.0, SliceDepth * 0.05), 0.05);
    }
    float LightNearPlane = SliceLSMin.z - ZPadding;
    float LightFarPlane  = SliceLSMax.z + ZPadding;

    if (LightFarPlane <= LightNearPlane)
    {
        LightFarPlane = LightNearPlane + 1.0;
    }

    // Create the projection
    float4x4 Projection = Matrix::OrthographicProjection(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, LightNearPlane, LightFarPlane);

    // Create the final view-projection matrix after we have stabilized the projection matrix
    float4x4 ViewProjection = mul(View, Projection);

    // Create inverse matrices
    float4x4 InvProjection     = Matrix::InvScaleTranslation(Projection);
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
        Split.RefWorldTexelSize     = ReferenceWorldTexelSize;
        Split.PCFMarginTexels       = PCFMarginTexels;
        Split.PCSSMarginTexels      = PCSSMarginTexels;
        Split.MaxPCSSRadiusTexels   = MaxPCSSRadiusTexels;
        Split.MaxPCSSRadiusWorld    = MaxPCSSRadiusTexels * ReferenceWorldTexelSize;
        Split.MaxPCSSSearchWorld    = Split.MaxPCSSRadiusWorld;
        Split.TransitionWidthViewZ  = 2.0 * (SelectedMarginTexels * ReferenceWorldTexelSize);
        Split.TransitionMarginTexels = SelectedMarginTexels;
        Split.CascadeUpdatedThisFrame = CascadeUpdatedThisFrame;
        Split.Padding1 = 0.0;
        Split.Padding2 = 0.0;
        Split.Padding3 = 0.0;
        Split.Padding4 = 0.0;

        SplitBuffer[CascadeIndex] = Split;
    }
}
