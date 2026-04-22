#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorEngine;

struct FLayoutIds
{
    uint32 Dockspace        = 0; // Root dockspace
    uint32 DockRight        = 0; // Right column
    uint32 DockRightTop     = 0; // Right column top (Scene hiearchy)
    uint32 DockRightBottom  = 0; // Right column bottom (Details)
    uint32 DockCenter       = 0; // Center area
    uint32 DockCenterTop    = 0; // Center top (Viewport)
    uint32 DockCenterBottom = 0; // Center bottom (Output Log)
    uint32 DockLeftTop      = 0; // Left column (Renderer settings)
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