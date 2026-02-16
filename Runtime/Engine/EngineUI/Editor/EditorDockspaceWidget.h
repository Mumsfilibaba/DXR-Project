#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorEngine;

struct FLayoutIds
{
    uint32 Dockspace        = 0; // Root dockspace
    uint32 DockRight        = 0; // Right column
    uint32 DockRightTop     = 0; // Right column top (Scene hiearchy)
    uint32 DockRightBottom  = 0; // Right column bottom (Details)
    uint32 DockLeftTop      = 0; // Left top (Renderer Settings)
    uint32 DockCenter       = 0; // Main area (Left + Center combined)
    uint32 DockCenterTop    = 0; // Top row (Viewport area after left split)
    uint32 DockCenterBottom = 0; // Bottom row (Content Browser + Output Log)
};

class FEditorDockspaceWidget
{
public:
    FEditorDockspaceWidget(FEditorEngine* InEditorEngine);
    ~FEditorDockspaceWidget();

    bool InitializeEditorStyle();
    void BuildDockingLayout(FLayoutIds& Ids);

    void Draw();
    void DrawMenuBar();
    void DrawDockSpace();
    void DrawFooter();

private:
    FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiDelegateHandle;
    FLayoutIds      LayoutIds;
    bool            bResetLayout;
};
