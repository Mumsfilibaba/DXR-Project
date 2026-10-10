#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "RHI/RHICore.h"

struct FStaticMeshProxyUpdate
{
    FMatrix4 TransformMatrix        = {};
    FMatrix4 TransformMatrixInverse = {};
};

struct FDirectionalLightProxyUpdate
{
    FVector3 Color                      = {};
    FVector3 Direction                  = {};
    float    ShadowNearPlane            = 0.0f;
    float    ShadowFarPlane             = 0.0f;
    float    ShadowBias                 = 0.0f;
    float    ShadowPositionOffset       = 0.0f;
    float    CascadeSplitLambda         = 0.0f;
    float    LightArea                  = 0.0f;
    bool     bCastShadows               = true;
};

struct FPointLightProxyUpdate
{
    FVector3 Position                           = {};
    FVector3 Color                              = {};
    float    ShadowBias                         = 0.0f;
    float    ShadowNearPlane                    = 0.0f;
    float    ShadowFarPlane                     = 0.0f;
    FMatrix4 ViewMatrix[RHI_NUM_CUBE_FACES]     = {};
    FMatrix4 ProjMatrix[RHI_NUM_CUBE_FACES]     = {};
    FMatrix4 ViewProjMatrix[RHI_NUM_CUBE_FACES] = {};
    bool     bCastShadows                       = true;
};

struct FLightProbeProxyUpdate
{
    FVector3 Position       = {};
    FVector3 BoxOffset      = {};
    FVector3 BoxExtent      = {};
    bool     bBoxProjection = false;
};

struct FRenderUpdateBatch
{
    TArray<FStaticMeshProxyUpdate> StaticMeshUpdates;
    FDirectionalLightProxyUpdate   DirectionalLight;
    TArray<FPointLightProxyUpdate> PointLightUpdates;
    TArray<FLightProbeProxyUpdate> LightProbeUpdates;
    bool                           bHasDirectionalLight = false;
};
