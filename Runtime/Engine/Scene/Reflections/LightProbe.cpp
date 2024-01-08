#include "LightProbe.h"

FOBJECT_IMPLEMENT_CLASS(FLightProbe);

FLightProbe::FLightProbe(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , Position()
    , Bounds()
{
}

void FLightProbe::SetPosition(const FVector3& InPosition)
{
    Position = InPosition;
}

void FLightProbe::SetBounds(const FAABB& InBounds)
{
    Bounds = InBounds;
}
