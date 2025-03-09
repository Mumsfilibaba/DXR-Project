#include "SkyboxComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSkyboxComponent);

FSkyboxComponent::FSkyboxComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , CubeMap(nullptr)
{
}

void FSkyboxComponent::SetCubeMap(const FRHITextureRef& InCubeMap)
{
    CubeMap = InCubeMap;
}