#pragma once
#include "Core/CoreDefines.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

struct IConsoleVariable;

class FEditorRendererSettingsWidget
{
public:
    FEditorRendererSettingsWidget();
    ~FEditorRendererSettingsWidget();

    void Draw();

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    void DrawWindow();

    void CaptureDefaultsIfNeeded();

    IConsoleVariable* GetCachedConsoleVariable(const CHAR* InName);

    void DrawDeferredRenderingSettings();
    void DrawShadowSettings();
    void DrawCascadedShadowSettings();
    void DrawPointLightShadowSettings();
    void DrawSkyboxSettings();
    void DrawSSAOSettings();
    void DrawRayTracingSettings();
    void DrawRayTracingReflectionsSettings();
    void DrawTAASettings();
    void DrawFXAASettings();
    void DrawDisplaySettings();
    void DrawCullingSettings();
    void DrawDebugSettings();
    void DrawTonemappingSettings();

    FDelegateHandle                      ImGuiDelegateHandle;
    TMap<String, bool>                   BoolDefaults;
    TMap<String, int32>                  IntDefaults;
    TMap<String, float>                  FloatDefaults;
    TMap<const CHAR*, IConsoleVariable*> CVarCache;
    bool                                 bVisible;
    bool                                 bDefaultsCaptured;
};
