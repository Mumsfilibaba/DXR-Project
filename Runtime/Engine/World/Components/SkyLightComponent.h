#pragma once
#include "Engine/Resources/Environment.h"
#include "Engine/World/Components/LightComponent.h"

enum class ESkyLightSource : uint8
{
    /** @brief Light the scene with the environment of the world's skybox, following it when it changes */
    FollowSkybox = 0,

    /** @brief Light the scene with an environment of its own, such as one without the sun a directional light already gives */
    SpecifiedEnvironment = 1,
};

class ENGINE_API FSkyLightComponent : public FLightComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSkyLightComponent, FLightComponent);

    FSkyLightComponent(const FObjectInitializer& ObjectInitializer);
    ~FSkyLightComponent();

    void SetSource(ESkyLightSource InSource);
    void SetEnvironment(const TSharedPtr<FEnvironment>& InEnvironment);
    void SetRotation(float InRotation);

    FORCEINLINE ESkyLightSource GetSource() const
    {
        return Source;
    }

    FORCEINLINE const TSharedPtr<FEnvironment>& GetEnvironment() const
    {
        return Environment;
    }

    FORCEINLINE float GetRotation() const
    {
        return Rotation;
    }

private:
    ESkyLightSource          Source;
    TSharedPtr<FEnvironment> Environment;
    float                    Rotation;
};
