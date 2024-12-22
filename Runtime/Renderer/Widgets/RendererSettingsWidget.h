#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FRendererSettingsWidget
{
public:
    FRendererSettingsWidget();
    ~FRendererSettingsWidget();

    void Draw();

private:
    void DrawDeferredRenderingSettings();
    void DrawShadowSettings();
    void DrawCascadedShadowSettings();
    void DrawPointLightShadowSettings();
    void DrawSkyboxSettings();
    void DrawSSAOSettings();
    void DrawTAASettings();
    void DrawFXAASettings();
    void DrawOtherSettings();
    void DrawDebugSettings();

    FDelegateHandle ImGuiDelegateHandle;
};
