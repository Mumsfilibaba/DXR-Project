#pragma once
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector3.h"

struct FCameraSnapshot
{
    FMatrix4 View                        = {};
    FMatrix4 ViewInverse                 = {};
    FMatrix4 Projection                  = {};
    FMatrix4 ProjectionInverse           = {};
    FMatrix4 ViewProjection              = {};
    FMatrix4 ViewProjectionInverse       = {};
    FMatrix4 ViewProjectionNoTranslation = {};
    FVector3 Position                    = {};
    FVector3 Forward                     = {};
    FVector3 Right                       = {};
    FVector3 Up                          = {};
    float    NearPlane                   = 0.0f;
    float    FarPlane                    = 0.0f;
    float    AspectRatio                 = 0.0f;
};
