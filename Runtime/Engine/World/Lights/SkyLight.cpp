#include "Engine/World/Lights/SkyLight.h"

FOBJECT_IMPLEMENT_CLASS(FSkyLight);

FSkyLight::FSkyLight(const FObjectInitializer& ObjectInitializer)
    : FLight(ObjectInitializer)
    , CubeMap(nullptr)
{
}

void FSkyLight::SetCubeMap(const FRHITextureRef& InCubeMap)
{
    CubeMap = InCubeMap;
}
