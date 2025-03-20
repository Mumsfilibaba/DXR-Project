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
    void SetPosition(const FVector3& InPosition);

    // Sets the extents of the box
    void SetBoxExtent(const FVector3& InBoxExtent);

    // Sets the offset of the box relative to the probe position
    void SetBoxOffset(const FVector3& InBoxExtent);

    // Set if box-projection should be enabled or not
    void SetBoxProjection(bool bInBoxProjection);

    // Set the source cube-map
    void SetCubeMap(const FRHITextureRef& InCubeMap);

    FORCEINLINE const FVector3& GetPosition()
    {
        return Position;
    }

    FORCEINLINE const FVector3& GetBoxOffset()
    {
        return BoxOffset;
    }

    FORCEINLINE const FVector3& GetBoxExtents()
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
    FVector3       Position;
    FVector3       BoxOffset;
    FVector3       BoxExtent;
    bool           bBoxProjection;
    FRHITextureRef CubeMap;
};
