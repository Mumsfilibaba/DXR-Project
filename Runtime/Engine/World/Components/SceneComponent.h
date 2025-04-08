#pragma once
#include "Engine/World/Components/ActorComponent.h"

class ENGINE_API FSceneComponent : public FActorComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSceneComponent, FActorComponent);

    FSceneComponent(const FObjectInitializer& ObjectInitializer);
    virtual ~FSceneComponent();
};