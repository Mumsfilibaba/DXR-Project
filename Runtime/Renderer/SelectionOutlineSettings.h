#pragma once
#include "Core/Core.h"
#include "Core/Math/Vector3.h"

struct FSelectionOutlineSettings
{
    bool     bEnabled    = true;
    int32    ThicknessPx = 3;
    float    Alpha       = 0.75f;
    float    Smoothness  = 1.0f;
    FVector3 Color       = FVector3(1.0f, 0.6f, 0.0f);
};

FSelectionOutlineSettings GetSelectionOutlineSettings();
