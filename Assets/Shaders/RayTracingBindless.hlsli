#ifndef RAY_TRACING_BINDLESS_HLSLI
#define RAY_TRACING_BINDLESS_HLSLI

#define RT_MAX_POINT_LIGHTS 8

struct FRayTracingSceneConstants
{
    // 0-16
    uint FrameIndex;
    uint NumSkyLightMips;  // Mip count of the specular cube-map
    uint NumPointLights;   // Number of valid entries in the point-light arrays
    uint Padding0;
    // 16-32
    float3 SunDirection;
    float  SunPadding0;
    // 32-48
    float3 SunColor;
    float  SunPadding1;
    // 48-176
    float4 PointLightPositionRadius[RT_MAX_POINT_LIGHTS];
    // 176-304
    float4 PointLightColor[RT_MAX_POINT_LIGHTS];
    // 304-320
    float  ReflectionMaxRayDistance;
    float  ReflectionMirrorRoughnessThreshold;
    float  ReflectionRayBias;
    float  Padding1;

    // 320-336
    uint   ReflectionSampler;   // One of REFLECTION_SAMPLER_*
    uint   ReflectionNoiseSize; // Edge length of the blue noise mask; 0 when no mask is bound
    uint   Padding2;
    uint   Padding3;
};

struct FRayTracingGeometryIndices
{
    // 0-16
    uint  VerticesHandle;
    uint  IndicesHandle;
    uint  MaterialIndex;
    float DeterminantSign;
};

#endif
