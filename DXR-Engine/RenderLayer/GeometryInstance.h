#pragma once
#include "Rendering/Resources/Material.h"

#include <Containers/SharedPtr.h>

struct RayTracingGeometryInstance
{
    RayTracingGeometryInstance(const TSharedRef<RayTracingGeometry>& InInstance, Material* InMaterial, const XMFLOAT3X4& InTransform)
        : Instance(InInstance)
        , Material(InMaterial)
        , Transform(InTransform)
    {
    }

    TSharedRef<RayTracingGeometry> Instance;
    Material*  Material;
    XMFLOAT3X4 Transform;
};
