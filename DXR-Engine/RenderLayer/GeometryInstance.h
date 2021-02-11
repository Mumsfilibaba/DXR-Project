#pragma once
#include "Rendering/Resources/Material.h"

#include <Containers/SharedPtr.h>

struct RayTracingGeometryInstance
{
    RayTracingGeometryInstance(const TRef<RayTracingGeometry>& InInstance, Material* InMaterial, const XMFLOAT3X4& InTransform)
        : Instance(InInstance)
        , Material(InMaterial)
        , Transform(InTransform)
    {
    }

    TRef<RayTracingGeometry> Instance;
    Material*  Material;
    XMFLOAT3X4 Transform;
};
