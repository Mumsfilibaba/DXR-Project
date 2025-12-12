#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

struct FLayoutIds
{
    uint32 Dockspace        = 0; // Root dockspace

    // Core columns/areas
    uint32 DockRight        = 0; // Right column (Outliner / Details)
    uint32 DockRightTop     = 0; // Outliner (top)
    uint32 DockProperties   = 0; // Details panel (currently == DockRightBottom)
    uint32 DockCenter       = 0; // Central (Viewport)
    uint32 DockLeft         = 0; // Left strip (Place Actors)

    // Bottom stack
    uint32 DockBottom       = 0; // Full bottom stack (both rows together)
    uint32 DockBottomMain   = 0; // Upper row of bottom (Content Browser, Output Log, etc.)

    // Footer
    uint32 DockFooter       = 0; // Footer (Console Input Bar)
};

class FEditorEngine;

class FEditorDockspaceWidget
{
public:
    FEditorDockspaceWidget(FEditorEngine* InEditorEngine);
    ~FEditorDockspaceWidget();

	void InitializeEditorStyle();
    void BuildDockingLayout(FLayoutIds& Ids);

	bool BeginDockspace(bool& bOutOpen);
	void EndDockspace();

	void DrawEngineWindows();
    void Draw();

private:
	FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiDelegateHandle;
	FLayoutIds      LayoutIds;
    bool            bResetLayout;
};