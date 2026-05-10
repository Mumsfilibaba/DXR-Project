#include "Renderer/EditorGridSettings.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"

// File-scope settings instance. The bound CVars below write directly into these fields,
// so the console / config files / command-line and the renderer all read/write the same memory.
static FEditorGridSettings GEditorGridSettings =
{
    /* bEnabled         */ true,
    /* PlaneY           */ 0.0f,
    /* MinorSize        */ 1.0f,
    /* MajorSize        */ 10.0f,
    /* MinorWidth       */ 1.0f,
    /* MajorWidth       */ 1.5f,
    /* MaxTraceDistance */ 0.0f,
    /* FadeDistance     */ 5000.0f,
    /* HorizonFade      */ 4.0f,
    /* DepthBias        */ 0.01f,
    /* MinorColor       */ FVector3(0.1f, 0.1f, 0.1f),
    /* MinorAlpha       */ 0.6f,
    /* MajorColor       */ FVector3(0.14f, 0.14f, 0.14f),
    /* MajorAlpha       */ 0.7f,
};

static FAutoConsoleVariableRef CVarEditorGridEnabled(
    "Renderer.Editor.Grid.Enable",
    "Enables editor viewport grid overlay (procedural infinite plane)",
    GEditorGridSettings.bEnabled,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridPlaneY(
    "Renderer.Editor.Grid.PlaneY",
    "World-space plane height for the grid (Y = PlaneY)",
    GEditorGridSettings.PlaneY,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMinorSize(
    "Renderer.Editor.Grid.MinorSize",
    "Minor grid spacing in world units (Meters)",
    GEditorGridSettings.MinorSize,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMajorSize(
    "Renderer.Editor.Grid.MajorSize",
    "Major grid spacing in world units (Meters)",
    GEditorGridSettings.MajorSize,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMinorWidth(
    "Renderer.Editor.Grid.MinorWidth",
    "Minor grid line width multiplier (anti-aliased using fwidth)",
    GEditorGridSettings.MinorWidth,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMajorWidth(
    "Renderer.Editor.Grid.MajorWidth",
    "Major grid line width multiplier (anti-aliased using fwidth)",
    GEditorGridSettings.MajorWidth,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMaxTraceDistance(
    "Renderer.Editor.Grid.MaxTraceDistance",
    "Maximum ray-plane trace distance (along the view ray). 0 = infinite",
    GEditorGridSettings.MaxTraceDistance,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridFadeDistance(
    "Renderer.Editor.Grid.FadeDistance",
    "Distance (along view ray) where the grid fades out (0 = no fade)",
    GEditorGridSettings.FadeDistance,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridHorizonFade(
    "Renderer.Editor.Grid.HorizonFade",
    "Horizon fade strength (multiplies abs(rayDir.y))",
    GEditorGridSettings.HorizonFade,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridDepthBias(
    "Renderer.Editor.Grid.DepthBias",
    "Depth compare bias (world units along view ray) to prevent grid bleeding through geometry",
    GEditorGridSettings.DepthBias,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMinorColorR(
    "Renderer.Editor.Grid.MinorColorR",
    "Minor grid color (linear) - Red channel (0-1)",
    GEditorGridSettings.MinorColor.X,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMinorColorG(
    "Renderer.Editor.Grid.MinorColorG",
    "Minor grid color (linear) - Green channel (0-1)",
    GEditorGridSettings.MinorColor.Y,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMinorColorB(
    "Renderer.Editor.Grid.MinorColorB",
    "Minor grid color (linear) - Blue channel (0-1)",
    GEditorGridSettings.MinorColor.Z,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMinorAlpha(
    "Renderer.Editor.Grid.MinorAlpha",
    "Minor grid alpha (0-1)",
    GEditorGridSettings.MinorAlpha,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMajorColorR(
    "Renderer.Editor.Grid.MajorColorR",
    "Major grid color (linear) - Red channel (0-1)",
    GEditorGridSettings.MajorColor.X,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMajorColorG(
    "Renderer.Editor.Grid.MajorColorG",
    "Major grid color (linear) - Green channel (0-1)",
    GEditorGridSettings.MajorColor.Y,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMajorColorB(
    "Renderer.Editor.Grid.MajorColorB",
    "Major grid color (linear) - Blue channel (0-1)",
    GEditorGridSettings.MajorColor.Z,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarEditorGridMajorAlpha(
    "Renderer.Editor.Grid.MajorAlpha",
    "Major grid alpha (0-1)",
    GEditorGridSettings.MajorAlpha,
    EConsoleVariableFlags::Default);

FEditorGridSettings GetEditorGridSettings()
{
    FEditorGridSettings Settings = GEditorGridSettings;

    Settings.MinorSize = Math::Clamp<float>(Settings.MinorSize, 0.001f, 1000000.0f);
    Settings.MajorSize = Math::Clamp<float>(Settings.MajorSize, 0.001f, 1000000.0f);
    if (Settings.MajorSize < Settings.MinorSize)
    {
        Settings.MajorSize = Settings.MinorSize;
    }

    Settings.MinorWidth = Math::Clamp<float>(Settings.MinorWidth, 0.25f, 16.0f);
    Settings.MajorWidth = Math::Clamp<float>(Settings.MajorWidth, 0.25f, 16.0f);

    Settings.MaxTraceDistance = Math::Clamp<float>(Settings.MaxTraceDistance, 0.0f, 10000000.0f);
    Settings.FadeDistance     = Math::Clamp<float>(Settings.FadeDistance, 0.0f, 10000000.0f);
    Settings.HorizonFade      = Math::Clamp<float>(Settings.HorizonFade, 0.0f, 128.0f);
    Settings.DepthBias        = Math::Clamp<float>(Settings.DepthBias, 0.0f, 1000.0f);

    Settings.MinorColor.X = Math::Clamp<float>(Settings.MinorColor.X, 0.0f, 1.0f);
    Settings.MinorColor.Y = Math::Clamp<float>(Settings.MinorColor.Y, 0.0f, 1.0f);
    Settings.MinorColor.Z = Math::Clamp<float>(Settings.MinorColor.Z, 0.0f, 1.0f);
    Settings.MinorAlpha   = Math::Clamp<float>(Settings.MinorAlpha, 0.0f, 1.0f);

    Settings.MajorColor.X = Math::Clamp<float>(Settings.MajorColor.X, 0.0f, 1.0f);
    Settings.MajorColor.Y = Math::Clamp<float>(Settings.MajorColor.Y, 0.0f, 1.0f);
    Settings.MajorColor.Z = Math::Clamp<float>(Settings.MajorColor.Z, 0.0f, 1.0f);
    Settings.MajorAlpha   = Math::Clamp<float>(Settings.MajorAlpha, 0.0f, 1.0f);

    return Settings;
}
