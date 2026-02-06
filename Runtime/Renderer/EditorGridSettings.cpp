#include "Renderer/EditorGridSettings.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"

static TAutoConsoleVariable<bool> CVarEditorGridEnabled(
    "Renderer.Editor.Grid.Enable",
    "Enables editor viewport grid overlay (procedural infinite plane)",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridPlaneY(
    "Renderer.Editor.Grid.PlaneY",
    "World-space plane height for the grid (Y = PlaneY)",
    0.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMinorSize(
    "Renderer.Editor.Grid.MinorSize",
    "Minor grid spacing in world units (Meters)",
    1.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMajorSize(
    "Renderer.Editor.Grid.MajorSize",
    "Major grid spacing in world units (Meters)",
    10.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMinorWidth(
    "Renderer.Editor.Grid.MinorWidth",
    "Minor grid line width multiplier (anti-aliased using fwidth)",
    1.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMajorWidth(
    "Renderer.Editor.Grid.MajorWidth",
    "Major grid line width multiplier (anti-aliased using fwidth)",
    1.5f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMaxTraceDistance(
    "Renderer.Editor.Grid.MaxTraceDistance",
    "Maximum ray-plane trace distance (along the view ray). 0 = infinite",
    0.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridFadeDistance(
    "Renderer.Editor.Grid.FadeDistance",
    "Distance (along view ray) where the grid fades out (0 = no fade)",
    5000.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridHorizonFade(
    "Renderer.Editor.Grid.HorizonFade",
    "Horizon fade strength (multiplies abs(rayDir.y))",
    4.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridDepthBias(
    "Renderer.Editor.Grid.DepthBias",
    "Depth compare bias (world units along view ray) to prevent grid bleeding through geometry",
    0.01f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMinorColorR(
    "Renderer.Editor.Grid.MinorColorR",
    "Minor grid color (linear) - Red channel (0-1)",
    0.1f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMinorColorG(
    "Renderer.Editor.Grid.MinorColorG",
    "Minor grid color (linear) - Green channel (0-1)",
    0.1f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMinorColorB(
    "Renderer.Editor.Grid.MinorColorB",
    "Minor grid color (linear) - Blue channel (0-1)",
    0.1f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMinorAlpha(
    "Renderer.Editor.Grid.MinorAlpha",
    "Minor grid alpha (0-1)",
    0.6f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMajorColorR(
    "Renderer.Editor.Grid.MajorColorR",
    "Major grid color (linear) - Red channel (0-1)",
    0.14f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMajorColorG(
    "Renderer.Editor.Grid.MajorColorG",
    "Major grid color (linear) - Green channel (0-1)",
    0.14f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMajorColorB(
    "Renderer.Editor.Grid.MajorColorB",
    "Major grid color (linear) - Blue channel (0-1)",
    0.14f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarEditorGridMajorAlpha(
    "Renderer.Editor.Grid.MajorAlpha",
    "Major grid alpha (0-1)",
    0.7f,
    EConsoleVariableFlags::Default);

FEditorGridSettings GetEditorGridSettings()
{
    FEditorGridSettings Settings;
    Settings.bEnabled = CVarEditorGridEnabled.GetValue();

    Settings.PlaneY = CVarEditorGridPlaneY.GetValue();

    Settings.MinorSize = Math::Clamp<float>(CVarEditorGridMinorSize.GetValue(), 0.001f, 1000000.0f);
    Settings.MajorSize = Math::Clamp<float>(CVarEditorGridMajorSize.GetValue(), 0.001f, 1000000.0f);
    if (Settings.MajorSize < Settings.MinorSize)
    {
        Settings.MajorSize = Settings.MinorSize;
    }

    Settings.MinorWidth = Math::Clamp<float>(CVarEditorGridMinorWidth.GetValue(), 0.25f, 16.0f);
    Settings.MajorWidth = Math::Clamp<float>(CVarEditorGridMajorWidth.GetValue(), 0.25f, 16.0f);

    Settings.MaxTraceDistance = Math::Clamp<float>(CVarEditorGridMaxTraceDistance.GetValue(), 0.0f, 10000000.0f);
    Settings.FadeDistance = Math::Clamp<float>(CVarEditorGridFadeDistance.GetValue(), 0.0f, 10000000.0f);
    Settings.HorizonFade  = Math::Clamp<float>(CVarEditorGridHorizonFade.GetValue(), 0.0f, 128.0f);
    Settings.DepthBias    = Math::Clamp<float>(CVarEditorGridDepthBias.GetValue(), 0.0f, 1000.0f);

    const float MinorR = Math::Clamp<float>(CVarEditorGridMinorColorR.GetValue(), 0.0f, 1.0f);
    const float MinorG = Math::Clamp<float>(CVarEditorGridMinorColorG.GetValue(), 0.0f, 1.0f);
    const float MinorB = Math::Clamp<float>(CVarEditorGridMinorColorB.GetValue(), 0.0f, 1.0f);
    Settings.MinorColor  = FVector3(MinorR, MinorG, MinorB);
    Settings.MinorAlpha  = Math::Clamp<float>(CVarEditorGridMinorAlpha.GetValue(), 0.0f, 1.0f);

    const float MajorR = Math::Clamp<float>(CVarEditorGridMajorColorR.GetValue(), 0.0f, 1.0f);
    const float MajorG = Math::Clamp<float>(CVarEditorGridMajorColorG.GetValue(), 0.0f, 1.0f);
    const float MajorB = Math::Clamp<float>(CVarEditorGridMajorColorB.GetValue(), 0.0f, 1.0f);
    Settings.MajorColor  = FVector3(MajorR, MajorG, MajorB);
    Settings.MajorAlpha  = Math::Clamp<float>(CVarEditorGridMajorAlpha.GetValue(), 0.0f, 1.0f);

    return Settings;
}
