#include "LightProbe.h"

FOBJECT_IMPLEMENT_CLASS(FLightProbe);

FLightProbe::FLightProbe(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , Position()
    , BoxOffset()
    , BoxExtent(1.0f, 1.0f, 1.0f)
    , bBoxProjection(false)
    , CubeMap(nullptr)
{
}

FLightProbe::~FLightProbe()
{
}

void FLightProbe::SetPosition(const FVector3& InPosition)
{
    Position = InPosition;
}

void FLightProbe::SetBoxExtent(const FVector3& InBoxExtent)
{
    BoxExtent = InBoxExtent;
}

void FLightProbe::SetBoxOffset(const FVector3& InBoxOffset)
{
    BoxOffset = InBoxOffset;
}

void FLightProbe::SetBoxProjection(bool bInBoxProjection)
{
    bBoxProjection = bInBoxProjection;
}

void FLightProbe::SetCubeMap(const FRHITextureRef& InCubeMap)
{
    CubeMap = InCubeMap;
}