#ifndef PARALLAX_MAPPING_HLSLI
#define PARALLAX_MAPPING_HLSLI 1

float ParallaxSampleHeight(Texture2D<float> HeightTex, SamplerState HeightSampler, float2 TexCoords, float2 TexCoordsDx, float2 TexCoordsDy)
{
    return 1.0f - HeightTex.SampleGrad(HeightSampler, TexCoords, TexCoordsDx, TexCoordsDy);
}

float2 ParallaxMapUV(Texture2D<float> HeightTex, SamplerState HeightSampler, float2 TexCoords, float3 ViewDir, 
    float2 TexCoordsDx, float2 TexCoordsDy, float HeightScale, float MinLayers, float MaxLayers, out uint bDiscard)
{
    bDiscard = 0;

    if (HeightScale <= 0.0f)
    {
        return TexCoords;
    }

    // Avoid instability at grazing angles.
    const float ViewZ = max(ViewDir.z, 0.05f);

    MinLayers = max(MinLayers, 1.0f);
    MaxLayers = max(MaxLayers, MinLayers);

    const float NumLayers  = lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir)));
    const float LayerDepth = 1.0f / NumLayers;

    const float2 P              = (ViewDir.xy / ViewZ) * HeightScale;
    const float2 DeltaTexCoords = P / NumLayers;

    float2 CurrentTexCoords     = TexCoords;
    float CurrentDepthMapValue  = ParallaxSampleHeight(HeightTex, HeightSampler, CurrentTexCoords, TexCoordsDx, TexCoordsDy);

    float CurrentLayerDepth = 0.0f;
    while (CurrentLayerDepth < CurrentDepthMapValue)
    {
        CurrentTexCoords     -= DeltaTexCoords;
        CurrentDepthMapValue  = ParallaxSampleHeight(HeightTex, HeightSampler, CurrentTexCoords, TexCoordsDx, TexCoordsDy);
        CurrentLayerDepth    += LayerDepth;
    }

    const float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

    const float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    const float BeforeDepth = ParallaxSampleHeight(HeightTex, HeightSampler, PrevTexCoords, TexCoordsDx, TexCoordsDy) - CurrentLayerDepth + LayerDepth;

    const float Weight = AfterDepth / (AfterDepth - BeforeDepth);
    float2 FinalTexCoords = (PrevTexCoords * Weight) + (CurrentTexCoords * (1.0f - Weight));

    // Stable silhouette clip: discard only when clearly outside [0,1], otherwise clamp to avoid sparkle from tiny OOB fluctuations.
    const float2 UvFwidth   = abs(TexCoordsDx) + abs(TexCoordsDy);
    const float2 ClipMargin = UvFwidth * 0.5f;
    if (FinalTexCoords.x < -ClipMargin.x || FinalTexCoords.x > 1.0f + ClipMargin.x ||
        FinalTexCoords.y < -ClipMargin.y || FinalTexCoords.y > 1.0f + ClipMargin.y)
    {
        bDiscard = 1;
        return FinalTexCoords;
    }

    FinalTexCoords = saturate(FinalTexCoords);
    return FinalTexCoords;
}

#endif // PARALLAX_MAPPING_HLSLI

