#pragma once
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorEngine;
class FWindowElement;

class FEditorTitleBarWidget final
{
public:
    static void RefreshMenuBarHeight(FEditorEngine* InEditorEngine);

public:
    FEditorTitleBarWidget(FEditorEngine* InEditorEngine);
    ~FEditorTitleBarWidget();

    void Draw();

private:
    void DrawMenuButtons();
    void DrawCaptionButtons(const FWindowTitleBarMetrics& Metrics, const TSharedPtr<FWindowElement>& Window);
    void AddInteractiveRect(const ImVec2& Min, const ImVec2& Max);

    FEditorEngine*         EditorEngine;
    FWindowTitleBarRegions Regions;
    ImVec2                 ClientOrigin;
};
