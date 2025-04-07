#pragma once
#include "Engine/World/Components/Component.h"

class ENGINE_API FSceneComponent : public FComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSceneComponent, FComponent);

    FSceneComponent(const FObjectInitializer& ObjectInitializer);
    virtual ~FSceneComponent();
};