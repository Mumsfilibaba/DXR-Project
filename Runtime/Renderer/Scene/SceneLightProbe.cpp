#include "RHI/RHITexture.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneProxyData.h"

FSceneLightProbe::FSceneLightProbe(FScene* InScene)
    : FSceneObject(InScene)
    , SpecularCubeMap(nullptr)
    , DiffuseCubeMap(nullptr)
    , Origin()
    , BoxMin()
    , BoxMax()
    , Intensity(1.0f)
    , Rotation(0.0f)
    , bBoxProjection(false)
{
}

FSceneLightProbe::~FSceneLightProbe() = default;

void FSceneLightProbe::RenderThread_ApplyUpdate(const FLightProbeProxyUpdate& Update)
{
    SpecularCubeMap = Update.SpecularCubeMap;
    DiffuseCubeMap  = Update.DiffuseCubeMap;
    Intensity       = Update.Intensity;
    Rotation        = Update.Rotation;
    Origin          = Update.Position;
    bBoxProjection  = Update.bBoxProjection;

    FVector3 BoxMidPoint   = Origin + Update.BoxOffset;
    FVector3 BoxExtentHalf = Update.BoxExtent * 0.5f;

    BoxMin = BoxMidPoint - BoxExtentHalf;
    BoxMax = BoxMidPoint + BoxExtentHalf;
}
