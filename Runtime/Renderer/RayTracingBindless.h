#pragma once
#include "RHI/RHIResources.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

// Must match RT_MAX_POINT_LIGHTS in Assets/Shaders/RayTracingBindless.hlsli
#define RT_MAX_POINT_LIGHTS 8

// Mirrors RayTracingSceneConstants in Assets/Shaders/RayTracingBindless.hlsli.
struct FRayTracingSceneConstantsHLSL
{
    uint32  FrameIndex      = 0;
    uint32  NumSkyLightMips = 0;
    uint32  NumPointLights  = 0;
    uint32  Padding0        = 0;
    Vector3 SunDirection    = Vector3(0.0f, -1.0f, 0.0f);
    float   SunPadding0     = 0.0f;
    Vector3 SunColor        = Vector3(0.0f, 0.0f, 0.0f);
    float   SunPadding1     = 0.0f;
    Vector4 PointLightPositionRadius[RT_MAX_POINT_LIGHTS];
    Vector4 PointLightColor[RT_MAX_POINT_LIGHTS];
    float   ReflectionMaxRayDistance           = 10000.0f;
    float   ReflectionMirrorRoughnessThreshold = 0.05f;
    float   ReflectionRayBias                  = 0.02f;
    float   Padding1                           = 0.0f;
    uint32  ReflectionSampler                  = 0;
    uint32  ReflectionNoiseSize                = 0;
    uint32  Padding2                           = 0;
    uint32  Padding3                           = 0;
};

struct FRayTracingGeometryIndicesHLSL
{
    FRHIDescriptorHandle VerticesHandle  = {};
    FRHIDescriptorHandle IndicesHandle   = {};
    uint32               MaterialIndex   = 0;
    float                DeterminantSign = 1.0f; // -1 when the instance transform mirrors, matching FPerObject.
};

static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL layout");
static_assert(sizeof(FRayTracingSceneConstantsHLSL) == 336, "FRayTracingSceneConstantsHLSL must match the HLSL constant buffer layout");
static_assert(sizeof(FRayTracingGeometryIndicesHLSL) == 16, "FRayTracingGeometryIndicesHLSL must match the HLSL structured-buffer layout");
