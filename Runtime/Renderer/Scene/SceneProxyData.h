#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "RHI/RHICore.h"

struct FCameraSnapshot
{
    Matrix4 View                        = {};
    Matrix4 ViewInverse                 = {};
    Matrix4 Projection                  = {};
    Matrix4 ProjectionInverse           = {};
    Matrix4 ViewProjection              = {};
    Matrix4 ViewProjectionInverse       = {};
    Matrix4 ViewProjectionNoTranslation = {};
    Vector3 Position                    = {};
    Vector3 Forward                     = {};
    Vector3 Right                       = {};
    Vector3 Up                          = {};
    float   NearPlane                   = 0.0f;
    float   FarPlane                    = 0.0f;
    float   AspectRatio                 = 0.0f;
};

struct FStaticMeshProxyUpdate
{
    Matrix4 TransformMatrix        = {};
    Matrix4 TransformMatrixInverse = {};
};

struct FDirectionalLightProxyUpdate
{
    Vector3 Color                       = {};
    Vector3 Direction                   = {};
    float   ShadowNearPlane             = 0.0f;
    float   ShadowFarPlane              = 0.0f;
    float   ShadowBias                  = 0.0f;
    float   ShadowPositionOffset        = 0.0f;
    float   CascadeSplitLambda          = 0.0f;
    float   LightArea                   = 0.0f;
    Matrix4 CameraViewProjectionInverse = {}; 
};

struct FPointLightProxyUpdate
{
    Vector3 Position                           = {};
    Vector3 Color                              = {};
    float   ShadowBias                         = 0.0f;
    float   ShadowNearPlane                    = 0.0f;
    float   ShadowFarPlane                     = 0.0f;
    Matrix4 ViewMatrix[RHI_NUM_CUBE_FACES]     = {};
    Matrix4 ProjMatrix[RHI_NUM_CUBE_FACES]     = {};
    Matrix4 ViewProjMatrix[RHI_NUM_CUBE_FACES] = {};
};

struct FLightProbeProxyUpdate
{
    Vector3 Position       = {};
    Vector3 BoxOffset      = {};
    Vector3 BoxExtent      = {};
    bool    bBoxProjection = false;
};

struct FRenderUpdateBatch
{
    FCameraSnapshot                Camera;
    TArray<FStaticMeshProxyUpdate> StaticMeshUpdates;
    FDirectionalLightProxyUpdate   DirectionalLight;
    TArray<FPointLightProxyUpdate> PointLightUpdates;
    TArray<FLightProbeProxyUpdate> LightProbeUpdates;
    bool                           bHasCamera           = false;
    bool                           bHasDirectionalLight = false;
};
