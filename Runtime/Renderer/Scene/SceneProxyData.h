#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "RHI/RHICore.h"
#include "RHI/RHITexture.h"

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
    FRHITextureRef SpecularCubeMap = nullptr;
    FRHITextureRef DiffuseCubeMap  = nullptr;
    FVector3       Position        = {};
    FVector3       BoxOffset       = {};
    FVector3       BoxExtent       = {};
    float          Intensity       = 1.0f;
    float          Rotation        = 0.0f;
    bool           bBoxProjection  = false;
};

struct FSkyboxProxyUpdate
{
    FRHITextureRef CubeMap   = nullptr;
    float          Intensity = 1.0f;
    float          Rotation  = 0.0f;
};

struct FSkyLightProxyUpdate
{
    FRHITextureRef SpecularCubeMap = nullptr;
    FRHITextureRef DiffuseCubeMap  = nullptr;
    FVector3       Color           = {};
    float          Rotation        = 0.0f;
};

struct FRenderUpdateBatch
{
    TArray<FStaticMeshProxyUpdate> StaticMeshUpdates;
    FDirectionalLightProxyUpdate   DirectionalLight;
    TArray<FPointLightProxyUpdate> PointLightUpdates;
    TArray<FLightProbeProxyUpdate> LightProbeUpdates;
    FSkyboxProxyUpdate             Skybox;
    FSkyLightProxyUpdate           SkyLight;
    bool                           bHasDirectionalLight = false;
};
