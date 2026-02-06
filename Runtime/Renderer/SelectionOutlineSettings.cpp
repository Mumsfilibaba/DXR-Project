#include "Renderer/SelectionOutlineSettings.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"

static TAutoConsoleVariable<bool> CVarSelectionOutlineEnabled(
    "Renderer.Editor.SelectionOutline.Enable",
    "Enables screen-space selection outline in the editor",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarSelectionOutlineThicknessPx(
    "Renderer.Editor.SelectionOutline.ThicknessPx",
    "Selection outline thickness in pixels",
    2,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarSelectionOutlineAlpha(
    "Renderer.Editor.SelectionOutline.Alpha",
    "Selection outline alpha (0-1)",
    0.8f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarSelectionOutlineSmoothness(
    "Renderer.Editor.SelectionOutline.Smoothness",
    "Selection outline smoothing/AA amount (0 = off, 1 = default)",
    1.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarSelectionOutlineColorR(
    "Renderer.Editor.SelectionOutline.ColorR",
    "Selection outline color (linear) - Red channel (0-1)",
    1.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarSelectionOutlineColorG(
    "Renderer.Editor.SelectionOutline.ColorG",
    "Selection outline color (linear) - Green channel (0-1)",
    0.6f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarSelectionOutlineColorB(
    "Renderer.Editor.SelectionOutline.ColorB",
    "Selection outline color (linear) - Blue channel (0-1)",
    0.0f,
    EConsoleVariableFlags::Default);

FSelectionOutlineSettings GetSelectionOutlineSettings()
{
    const float R = Math::Clamp<float>(CVarSelectionOutlineColorR.GetValue(), 0.0f, 1.0f);
    const float G = Math::Clamp<float>(CVarSelectionOutlineColorG.GetValue(), 0.0f, 1.0f);
    const float B = Math::Clamp<float>(CVarSelectionOutlineColorB.GetValue(), 0.0f, 1.0f);

    FSelectionOutlineSettings Settings;
    Settings.bEnabled    = CVarSelectionOutlineEnabled.GetValue();
    Settings.ThicknessPx = Math::Clamp<int32>(CVarSelectionOutlineThicknessPx.GetValue(), 1, 16);
    Settings.Alpha       = Math::Clamp<float>(CVarSelectionOutlineAlpha.GetValue(), 0.0f, 1.0f);
    Settings.Smoothness  = Math::Clamp<float>(CVarSelectionOutlineSmoothness.GetValue(), 0.0f, 8.0f);
    Settings.Color       = FVector3(R, G, B);

    return Settings;
}
