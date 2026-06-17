#include "Renderer/SelectionOutlineSettings.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"

// File-scope settings instance. The bound CVars below write directly into these fields,
// so the console / config files / command-line and the renderer all read/write the same memory.
static FSelectionOutlineSettings GSelectionOutlineSettings =
{
    /* bEnabled    */ true,
    /* ThicknessPx */ 2,
    /* Alpha       */ 0.8f,
    /* Smoothness  */ 1.0f,
    /* Color       */ Vector3(1.0f, 0.6f, 0.0f),
};

static FAutoConsoleVariableRef CVarSelectionOutlineEnabled(
    "Renderer.Editor.SelectionOutline.Enable",
    "Enables screen-space selection outline in the editor",
    GSelectionOutlineSettings.bEnabled,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineThicknessPx(
    "Renderer.Editor.SelectionOutline.ThicknessPx",
    "Selection outline thickness in pixels",
    GSelectionOutlineSettings.ThicknessPx,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineAlpha(
    "Renderer.Editor.SelectionOutline.Alpha",
    "Selection outline alpha (0-1)",
    GSelectionOutlineSettings.Alpha,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineSmoothness(
    "Renderer.Editor.SelectionOutline.Smoothness",
    "Selection outline smoothing/AA amount (0 = off, 1 = default)",
    GSelectionOutlineSettings.Smoothness,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineColorR(
    "Renderer.Editor.SelectionOutline.ColorR",
    "Selection outline color (linear) - Red channel (0-1)",
    GSelectionOutlineSettings.Color.X,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineColorG(
    "Renderer.Editor.SelectionOutline.ColorG",
    "Selection outline color (linear) - Green channel (0-1)",
    GSelectionOutlineSettings.Color.Y,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineColorB(
    "Renderer.Editor.SelectionOutline.ColorB",
    "Selection outline color (linear) - Blue channel (0-1)",
    GSelectionOutlineSettings.Color.Z,
    EConsoleVariableFlags::Default);

FSelectionOutlineSettings GetSelectionOutlineSettings()
{
    FSelectionOutlineSettings Settings = GSelectionOutlineSettings;
    Settings.ThicknessPx = Math::Clamp<int32>(Settings.ThicknessPx, 1, 16);
    Settings.Alpha       = Math::Clamp<float>(Settings.Alpha, 0.0f, 1.0f);
    Settings.Smoothness  = Math::Clamp<float>(Settings.Smoothness, 0.0f, 8.0f);
    Settings.Color.X     = Math::Clamp<float>(Settings.Color.X, 0.0f, 1.0f);
    Settings.Color.Y     = Math::Clamp<float>(Settings.Color.Y, 0.0f, 1.0f);
    Settings.Color.Z     = Math::Clamp<float>(Settings.Color.Z, 0.0f, 1.0f);
    return Settings;
}
