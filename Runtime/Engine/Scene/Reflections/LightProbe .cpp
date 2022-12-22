#include "LightProbe.h"

FLightProbe::FLightProbe()
    : FObject()
    , Position()
    , Bounds()
{
    FOBJECT_INIT();
}

void FLightProbe::SetPosition(const FVector3& InPosition)
{
    Position = InPosition;
}

void FLightProbe::SetBounds(const FAABB& InBounds)
{
    Bounds = InBounds;
}
