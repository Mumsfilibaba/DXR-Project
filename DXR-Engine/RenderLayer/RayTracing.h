#pragma once
#include "Resources.h"

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