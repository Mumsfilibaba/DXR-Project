#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FRendererSettingsWidget
{
public:
    FRendererSettingsWidget();
    ~FRendererSettingsWidget();

    void Draw();
    void DrawWindow();

private:
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
};
