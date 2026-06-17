#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/AABB.h"
#include "Engine/Core/Object.h"

class ENGINE_API FLightProbe : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FLightProbe, FObject);

    FLightProbe(const FObjectInitializer& ObjectInitializer);
    ~FLightProbe();

    // Set the position of the probe (Where the environment is captured)
    void SetPosition(const Vector3& InPosition);

    // Sets the extents of the box
    void SetBoxExtent(const Vector3& InBoxExtent);

    // Sets the offset of the box relative to the probe position
    void SetBoxOffset(const Vector3& InBoxExtent);

    // Set if box-projection should be enabled or not
    void SetBoxProjection(bool bInBoxProjection);

    // Set the source cube-map
    void SetCubeMap(const FRHITextureRef& InCubeMap);

    FORCEINLINE const Vector3& GetPosition()
    {
        return Position;
    }

    FORCEINLINE const Vector3& GetBoxOffset()
    {
        return BoxOffset;
    }

    FORCEINLINE const Vector3& GetBoxExtents()
    {
        return BoxExtent;
    }

    FORCEINLINE bool GetBoxProjection() const
    {
        return bBoxProjection;
    }

    FORCEINLINE const FRHITextureRef& GetCubeMap() const
    {
        return CubeMap;
    }

protected:
    FRHITextureRef CubeMap;
    Vector3        Position;
    Vector3        BoxOffset;
    Vector3        BoxExtent;
    bool           bBoxProjection;
};
