#pragma once
#include "Component.h"

class FProxyRendererComponent;

class ENGINE_API FRendererComponent : public FComponent
{
public:
    FOBJECT_DECLARE_CLASS(FRendererComponent, FComponent);

    FRendererComponent(const FObjectInitializer& ObjectInitializer);
    ~FRendererComponent() = default;

    virtual FProxyRendererComponent* CreateProxyComponent() 
    {
        return nullptr;
    }
};