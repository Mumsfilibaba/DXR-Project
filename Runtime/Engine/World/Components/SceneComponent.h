#pragma once
#include "Component.h"

class FProxySceneComponent;

class ENGINE_API FSceneComponent : public FComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSceneComponent, FComponent);

    FSceneComponent(const FObjectInitializer& ObjectInitializer);
    ~FSceneComponent() = default;

    virtual FProxySceneComponent* CreateProxyComponent() 
    {
        return nullptr;
    }
};