#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/AABB.h"

#include "Engine/CoreObject/CoreObject.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FLightProbe

class ENGINE_API FLightProbe
    : public FCoreObject
{
    CORE_OBJECT(FLightProbe, FCoreObject);

public:
    FLightProbe();
    ~FLightProbe() = default;

    void SetPosition(const FVector3& InPosition);
    void SetBounds(const FAABB& InBounds);

    FORCEINLINE const FVector3& GetPosition()
    {
        return Position;
    }

    FORCEINLINE const FAABB& GetBounds()
    {
        return Bounds;
    }

protected:
    FVector3 Position;
    FAABB    Bounds;
};