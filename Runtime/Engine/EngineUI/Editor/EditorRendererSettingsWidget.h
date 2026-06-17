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

    FDelegateHandle     ImGuiDelegateHandle;
    TMap<String, bool>  BoolDefaults;
    TMap<String, int32> IntDefaults;
    TMap<String, float> FloatDefaults;
    bool                bVisible          = true;
    bool                bDefaultsCaptured = false;
};
