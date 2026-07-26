#ifndef IMAGE_BASED_LIGHTING_HLSLI
#define IMAGE_BASED_LIGHTING_HLSLI

#ifndef ENABLE_LIGHT_PROBE_BOX_PROJECTION
    #define ENABLE_LIGHT_PROBE_BOX_PROJECTION 1
#endif

// ------------------------------------------------------------------------------------------------
// Specular Environment
// ------------------------------------------------------------------------------------------------

struct FSpecularEnvironmentInfo
{
    float3 ReflectionUVW;
    float  Roughness;
};

float3 SpecularEnvironment(TextureCube<float4> SpecularCubeMap, SamplerState EnvironmentSampler, FSpecularEnvironmentInfo EnvironmentInfo, int NumSkyLightMips)
{
    // Use a modified version of roughness when selecting miplevels
    float ModifiedRoughness = EnvironmentInfo.Roughness;
    ModifiedRoughness *= 1.7 - (0.7 * ModifiedRoughness);

    // Calculate the miplevel that we want to sample
    const float SpecularMipLevel = ModifiedRoughness * ((float)(NumSkyLightMips) - 1.0);

    // Sample and return specular cube-map
    return SpecularCubeMap.SampleLevel(EnvironmentSampler, EnvironmentInfo.ReflectionUVW, SpecularMipLevel).rgb;
}

float2 GetIntegrationConstants(Texture2D<float2> IntegrationLUT, SamplerState LUTSampler, float NDotV, float Roughness)
{
    return IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, Roughness), 0.0).rg;
}

// ------------------------------------------------------------------------------------------------
// Diffuse Environment
// ------------------------------------------------------------------------------------------------

struct FDiffuseEnvironmentInfo
{
    float3 NormalUVW;
};

float3 DiffuseEnvironment(TextureCube<float4> DiffuseCubeMap, SamplerState EnvironmentSampler, FDiffuseEnvironmentInfo EnvironmentInfo)
{
    // Sample and return the diffuse cube-map
    return DiffuseCubeMap.SampleLevel(EnvironmentSampler, EnvironmentInfo.NormalUVW, 0.0).rgb;
}

// ------------------------------------------------------------------------------------------------
// Box-Projection
// ------------------------------------------------------------------------------------------------

struct FBoxProjectionInfo
{
    float3 ReflectionUVW;
    float3 PositionWS;
    float3 CubeMapPositionWS;
    float3 BoxMinWS;
    float3 BoxMaxWS;
    float  BoxProjection;
};

float3 BoxProjection(FBoxProjectionInfo BoxProjectionInfo)
{
#if ENABLE_LIGHT_PROBE_BOX_PROJECTION
    [[branch]]
    if (BoxProjectionInfo.BoxProjection > 0.0)
    {
        const float3 ReflectionUVW   = BoxProjectionInfo.ReflectionUVW;
        const float3 Position        = BoxProjectionInfo.PositionWS;        // viewer's position
        const float3 CubeMapPosition = BoxProjectionInfo.CubeMapPositionWS; // probe's world position

        // Compute the relative positions from the viewer.
        float3 RelativeMin = BoxProjectionInfo.BoxMinWS - Position;
        float3 RelativeMax = BoxProjectionInfo.BoxMaxWS - Position;

        float x = (ReflectionUVW.x > 0 ? RelativeMax.x : RelativeMin.x) / ReflectionUVW.x;
        float y = (ReflectionUVW.y > 0 ? RelativeMax.y : RelativeMin.y) / ReflectionUVW.y;
        float z = (ReflectionUVW.z > 0 ? RelativeMax.z : RelativeMin.z) / ReflectionUVW.z;

        float Scalar = min(min(x, y), z);

        // Return the new sampling direction.
        return ReflectionUVW * Scalar + (Position - CubeMapPosition);
    }
    else
#endif
    {
        return BoxProjectionInfo.ReflectionUVW;
    }
}

bool IsInsideAABB(float3 Position, float3 BoxMin, float3 BoxMax)
{
    return 
        (Position.x >= BoxMin.x && Position.x <= BoxMax.x) && 
        (Position.y >= BoxMin.y && Position.y <= BoxMax.y) && 
        (Position.z >= BoxMin.z && Position.z <= BoxMax.z);
}

#endif
