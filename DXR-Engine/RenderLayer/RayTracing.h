#pragma once
#include "ResourceBase.h"

enum ERayTracingStructureFlag
{
    RayTracingStructureFlag_None            = 0x0,
    RayTracingStructureFlag_AllowUpdate     = FLAG(1),
    RayTracingStructureFlag_PreferFastTrace = FLAG(2),
    RayTracingStructureFlag_PreferFastBuild = FLAG(3),
};

// RayTracing Geometry (Bottom Level Acceleration Structure)
class RayTracingGeometry : public Resource
{
};

// RayTracing Scene (Top Level Acceleration Structure)
class RayTracingScene : public Resource
{
};

// RayTracingGeometryInstance
struct RayTracingGeometryInstance
{
    TSharedRef<RayTracingGeometry> Geometry;
    XMFLOAT3X4 TransformMatrix;
};