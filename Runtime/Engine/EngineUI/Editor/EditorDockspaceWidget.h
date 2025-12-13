#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

struct FLayoutIds
{
    uint32 Dockspace        = 0; // Root dockspace
    uint32 DockRight        = 0; // Right column
    uint32 DockRightTop     = 0; // Right column top (Scene hiearchy)
    uint32 DockRightBottom  = 0; // Right column bottom (Details)
    uint32 DockCenter       = 0; // Center area
    uint32 DockCenterTop    = 0; // Center top (Viewport)
    uint32 DockCenterBottom = 0; // Center bottom (Output Log)
};

class FEditorEngine;

class FEditorDockspaceWidget
{
public:
    FEditorDockspaceWidget(FEditorEngine* InEditorEngine);
    ~FEditorDockspaceWidget();

	void InitializeEditorStyle();
    void BuildDockingLayout(FLayoutIds& Ids);

    void Draw();
    void DrawMenuBar();
    void DrawDockSpace();
    void DrawConsole();

    // TODO: All engine windows should be their seperate thing
	void DrawEngineWindows();

private:
	FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiDelegateHandle;
	FLayoutIds      LayoutIds;
    bool            bResetLayout;
};