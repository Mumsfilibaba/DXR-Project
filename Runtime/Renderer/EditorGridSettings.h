#pragma once
#include "Core/Core.h"
#include "Core/Math/Vector3.h"

struct FEditorGridSettings
{
    bool  bEnabled      = true;
    float PlaneY        = 0.0f;
    float MinorSize     = 1.0f;
    float MajorSize     = 10.0f;
    float MinorWidth    = 1.75f;
    float MajorWidth    = 2.5f;
    float MaxTraceDistance = 0.0f;
    float FadeDistance  = 5000.0f;
    float HorizonFade   = 4.0f;
    float DepthBias     = 0.01f;

    FVector3 MinorColor = FVector3(0.14f, 0.14f, 0.14f);
    float    MinorAlpha = 0.24f;

    FVector3 MajorColor = FVector3(0.24f, 0.24f, 0.24f);
    float    MajorAlpha = 0.45f;
};

FEditorGridSettings GetEditorGridSettings();
