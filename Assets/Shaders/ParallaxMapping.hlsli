#ifndef PARALLAX_MAPPING_HLSLI
#define PARALLAX_MAPPING_HLSLI 1

float ParallaxSampleHeight(Texture2D<float> HeightTex, SamplerState HeightSampler, float2 TexCoords, float2 TexCoordsDx, float2 TexCoordsDy)
{
    return saturate(1.0f - HeightTex.SampleGrad(HeightSampler, TexCoords, TexCoordsDx, TexCoordsDy));
}

float2 ParallaxMapUV(Texture2D<float> HeightTex, SamplerState HeightSampler, float2 TexCoords, float3 ViewDir, 
    float2 TexCoordsDx, float2 TexCoordsDy, float HeightScale, float MinLayers, float MaxLayers, out bool bDiscard)
{
    bDiscard = false;

    // Reject non-finite or non-positive height scales so garbage material data can never enter the loop.
    if (!(HeightScale > 0.0f) || isinf(HeightScale))
    {
        return TexCoords;
    }

    // Avoid instability at grazing angles.
    const float ViewZ = max(ViewDir.z, 0.05f);

    MinLayers = clamp(MinLayers, 1.0f, 256.0f);
    MaxLayers = clamp(MaxLayers, MinLayers, 256.0f);

    const float  NumLayers         = clamp(lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir))), 1.0f, 256.0f);
    const float  LayerDepth        = 1.0f / NumLayers;
    const float2 MaxParallaxOffset = (ViewDir.xy / ViewZ) * HeightScale;
    const float2 DeltaTexCoords    = MaxParallaxOffset / NumLayers;
    const uint   MaxSteps          = (uint)NumLayers;

    float2 CurrentTexCoords     = TexCoords;
    float  CurrentDepthMapValue = ParallaxSampleHeight(HeightTex, HeightSampler, CurrentTexCoords, TexCoordsDx, TexCoordsDy);
    float  CurrentLayerDepth    = 0.0f;

    [loop]
    for (uint Step = 0; Step < MaxSteps && CurrentLayerDepth < CurrentDepthMapValue; ++Step)
    {
        CurrentTexCoords     -= DeltaTexCoords;
        CurrentDepthMapValue  = ParallaxSampleHeight(HeightTex, HeightSampler, CurrentTexCoords, TexCoordsDx, TexCoordsDy);
        CurrentLayerDepth    += LayerDepth;
    }

    const float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;
    const float  AfterDepth    = CurrentDepthMapValue - CurrentLayerDepth;
    const float  BeforeDepth   = ParallaxSampleHeight(HeightTex, HeightSampler, PrevTexCoords, TexCoordsDx, TexCoordsDy) - CurrentLayerDepth + LayerDepth;
    const float  Weight        = AfterDepth / (AfterDepth - BeforeDepth);

    float2 FinalTexCoords = (PrevTexCoords * Weight) + (CurrentTexCoords * (1.0f - Weight));

    const float2 UvFwidth   = abs(TexCoordsDx) + abs(TexCoordsDy);
    const float2 ClipMargin = UvFwidth * 0.5f;

    if (FinalTexCoords.x < -ClipMargin.x || FinalTexCoords.x > 1.0f + ClipMargin.x ||
        FinalTexCoords.y < -ClipMargin.y || FinalTexCoords.y > 1.0f + ClipMargin.y)
    {
        bDiscard = true;
        return FinalTexCoords;
    }

    FinalTexCoords = saturate(FinalTexCoords);
    return FinalTexCoords;
}

#endif // PARALLAX_MAPPING_HLSLI

