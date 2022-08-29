#include "LightProbe.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FLightProbe

FLightProbe::FLightProbe()
    : FCoreObject()
    , Position()
    , Bounds()
{
    CORE_OBJECT_INIT();
}

void FLightProbe::SetPosition(const FVector3& InPosition)
{
    Position = InPosition;
}

void FLightProbe::SetBounds(const FAABB& InBounds)
{
    Bounds = InBounds;
}
