#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

#include "Core/CoreDefines.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"

class FEditorRendererSettingsWidget
{
public:
    FEditorRendererSettingsWidget();
    ~FEditorRendererSettingsWidget();

    void Draw();

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:
    void DrawWindow();

    void CaptureDefaultsIfNeeded();

    void DrawDeferredRenderingSettings();
    void DrawShadowSettings();
    void DrawCascadedShadowSettings();
    void DrawPointLightShadowSettings();
    void DrawSkyboxSettings();
    void DrawSSAOSettings();
    void DrawTAASettings();
    void DrawFXAASettings();
    void DrawDisplaySettings();
    void DrawCullingSettings();
    void DrawDebugSettings();
    void DrawTonemappingSettings();

    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible = true;

    bool                 bDefaultsCaptured = false;
    TMap<FString, bool>  BoolDefaults;
    TMap<FString, int32> IntDefaults;
    TMap<FString, float> FloatDefaults;
};
