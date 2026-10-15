#pragma once
#include "RHI/RHIResources.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

// Must match RT_MAX_POINT_LIGHTS in Assets/Shaders/RayTracingBindless.hlsli
#define RT_MAX_POINT_LIGHTS 8

// Must match RT_GEOMETRY_FLAG_ALPHA_TESTED in Assets/Shaders/RayTracingBindless.hlsli
#define RT_GEOMETRY_FLAG_ALPHA_TESTED (1u)

// Mirrors RayTracingSceneConstants in Assets/Shaders/RayTracingBindless.hlsli.
struct FRayTracingSceneConstantsHLSL
{
    uint32   FrameIndex      = 0;
    uint32   NumSkyLightMips = 0;
    uint32   NumPointLights  = 0;
    uint32   Padding0        = 0;
    FVector3 SunDirection    = FVector3(0.0f, -1.0f, 0.0f);
    float    SunPadding0     = 0.0f;
    FVector3 SunColor        = FVector3(0.0f, 0.0f, 0.0f);
    float    SunPadding1     = 0.0f;
    FVector4 PointLightPositionRadius[RT_MAX_POINT_LIGHTS];
    FVector4 PointLightColor[RT_MAX_POINT_LIGHTS];
    float    ReflectionMaxRayDistance           = 10000.0f;
    float    ReflectionMirrorRoughnessThreshold = 0.05f;
    float    ReflectionRayBias                  = 0.02f;
    float    Padding1                           = 0.0f;
    uint32   ReflectionSampler                  = 0;
    uint32   ReflectionNoiseSize                = 0;
    uint32   Padding2                           = 0;
    uint32   Padding3                           = 0;
    FVector3 SkyLightColor                      = FVector3(1.0f, 1.0f, 1.0f);
    float    SkyLightRotation                   = 0.0f;
    float    SkyboxIntensity                    = 1.0f;
    float    SkyboxRotation                     = 0.0f;
    float    Padding4                           = 0.0f;
    float    Padding5                           = 0.0f;
};

struct FRayTracingGeometryIndicesHLSL
{
    FRHIDescriptorHandle AttributesHandle = {};
    FRHIDescriptorHandle IndicesHandle    = {};
    uint32               MaterialIndex    = 0;
    float                DeterminantSign  = 1.0f; // -1 when the instance transform mirrors, matching FPerObject.
    uint32               FirstTriangle    = 0;
    uint32               Flags            = 0;
};

static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL layout");
static_assert(sizeof(FRayTracingSceneConstantsHLSL) == 368, "FRayTracingSceneConstantsHLSL must match the HLSL constant buffer layout");
static_assert(sizeof(FRayTracingGeometryIndicesHLSL) == 24, "FRayTracingGeometryIndicesHLSL must match the HLSL structured-buffer layout");
