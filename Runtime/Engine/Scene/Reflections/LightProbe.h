#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/AABB.h"
#include "Engine/Core/Object.h"

class ENGINE_API FLightProbe : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FLightProbe, FObject, ENGINE_API);

    FLightProbe(const FObjectInitializer& ObjectInitializer);
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
