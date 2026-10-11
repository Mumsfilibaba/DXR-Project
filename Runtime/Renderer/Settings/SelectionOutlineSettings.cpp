#include "Renderer/Settings/SelectionOutlineSettings.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"

static FSelectionOutlineSettings GSelectionOutlineSettings =
{
    /* bEnabled    */ true,
    /* ThicknessPx */ 2,
    /* Alpha       */ 0.8f,
    /* Smoothness  */ 1.0f,
    /* Color       */ FVector3(1.0f, 0.6f, 0.0f),
};

static FAutoConsoleVariableRef CVarSelectionOutlineEnabled(
    TEXT("Renderer.Editor.SelectionOutline.Enable"),
    TEXT("Enables screen-space selection outline in the editor"),
    GSelectionOutlineSettings.bEnabled,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineThicknessPx(
    TEXT("Renderer.Editor.SelectionOutline.ThicknessPx"),
    TEXT("Selection outline thickness in pixels"),
    GSelectionOutlineSettings.ThicknessPx,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineAlpha(
    TEXT("Renderer.Editor.SelectionOutline.Alpha"),
    TEXT("Selection outline alpha (0-1)"),
    GSelectionOutlineSettings.Alpha,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineSmoothness(
    TEXT("Renderer.Editor.SelectionOutline.Smoothness"),
    TEXT("Selection outline smoothing/AA amount (0 = off, 1 = default)"),
    GSelectionOutlineSettings.Smoothness,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineColorR(
    TEXT("Renderer.Editor.SelectionOutline.ColorR"),
    TEXT("Selection outline color (linear) - Red channel (0-1)"),
    GSelectionOutlineSettings.Color.X,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineColorG(
    TEXT("Renderer.Editor.SelectionOutline.ColorG"),
    TEXT("Selection outline color (linear) - Green channel (0-1)"),
    GSelectionOutlineSettings.Color.Y,
    EConsoleVariableFlags::Default);

static FAutoConsoleVariableRef CVarSelectionOutlineColorB(
    TEXT("Renderer.Editor.SelectionOutline.ColorB"),
    TEXT("Selection outline color (linear) - Blue channel (0-1)"),
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
