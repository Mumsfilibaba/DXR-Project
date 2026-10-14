#pragma once
#include "Engine/Resources/Environment.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FSkyboxComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSkyboxComponent, FSceneComponent);

    FSkyboxComponent(const FObjectInitializer& ObjectInitializer);
    ~FSkyboxComponent() = default;

    void SetEnvironment(const TSharedPtr<FEnvironment>& InEnvironment);

    void SetIntensity(float InIntensity);

    void SetRotation(float InRotation);

    FORCEINLINE const TSharedPtr<FEnvironment>& GetEnvironment() const
    {
        return Environment;
    }

    FORCEINLINE float GetIntensity() const
    {
        return Intensity;
    }

    FORCEINLINE float GetRotation() const
    {
        return Rotation;
    }

private:
    TSharedPtr<FEnvironment> Environment;
    float                    Intensity;
    float                    Rotation;
};
