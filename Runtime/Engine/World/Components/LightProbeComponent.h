#pragma once
#include "Core/Math/Vector3.h"
#include "Engine/Resources/Environment.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FLightProbeComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FLightProbeComponent, FSceneComponent);

    FLightProbeComponent(const FObjectInitializer& ObjectInitializer);
    ~FLightProbeComponent();

    void SetBoxExtent(const FVector3& InBoxExtent);
    void SetBoxOffset(const FVector3& InBoxOffset);
    void SetBoxProjection(bool bInBoxProjection);

    void SetEnvironment(const TSharedPtr<FEnvironment>& InEnvironment);
    void SetIntensity(float InIntensity);

    void SetRotation(float InRotation);

    const FVector3& GetPosition() const;

    const FVector3& GetBoxOffset() const
    {
        return BoxOffset;
    }

    const FVector3& GetBoxExtents() const
    {
        return BoxExtent;
    }

    bool GetBoxProjection() const
    {
        return bBoxProjection;
    }

    const TSharedPtr<FEnvironment>& GetEnvironment() const
    {
        return Environment;
    }

    float GetIntensity() const
    {
        return Intensity;
    }

    float GetRotation() const
    {
        return Rotation;
    }

private:
    TSharedPtr<FEnvironment> Environment;
    FVector3                 BoxOffset;
    FVector3                 BoxExtent;
    float                    Intensity;
    float                    Rotation;
    bool                     bBoxProjection;
};
