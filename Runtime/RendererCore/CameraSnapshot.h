#pragma once
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector3.h"

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
