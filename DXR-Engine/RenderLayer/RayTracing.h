#pragma once
#include "ResourceBase.h"

#include <Containers/SharedPtr.h>

enum ERayTracingStructureBuildFlag
{
    RayTracingStructureBuildFlag_None            = 0x0,
    RayTracingStructureBuildFlag_AllowUpdate     = FLAG(1),
    RayTracingStructureBuildFlag_PreferFastTrace = FLAG(2),
    RayTracingStructureBuildFlag_PreferFastBuild = FLAG(3),
};

// RayTracing Geometry (Bottom Level Acceleration Structure)
class RayTracingGeometry : public Resource
{
public:
    RayTracingGeometry(UInt32 InFlags)
        : Flags(InFlags)
    {
    }

    ~RayTracingGeometry() = default;

    UInt32 GetFlags() const { return Flags; }

private:
    UInt32 Flags;
};

// RayTracing Scene (Top Level Acceleration Structure)
class RayTracingScene : public Resource
{
public:
    RayTracingScene(UInt32 InFlags)
        : Flags(InFlags)
    {
    }

    ~RayTracingScene() = default;

    UInt32 GetFlags() const { return Flags; }

private:
    UInt32 Flags;
};