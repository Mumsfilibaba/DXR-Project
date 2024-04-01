#include "RendererComponent.h"

FOBJECT_IMPLEMENT_CLASS(FRendererComponent);

FRendererComponent::FRendererComponent(const FObjectInitializer& ObjectInitializer)
    : FComponent(ObjectInitializer)
{
    SetTickable(false);
    SetStartable(false);
}