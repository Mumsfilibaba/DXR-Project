#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

struct FLayoutIds
{
	uint32 Dockspace       = 0;
	uint32 DockMain        = 0; // root
	uint32 DockRight       = 0; // right split
	uint32 DockRightBottom = 0; // details (bottom)
	uint32 DockRightTop    = 0; // outliner (top)
	uint32 DockBottom      = 0; // bottom split
	uint32 DockCenter      = 0; // central (viewport)
	uint32 DockLeft        = 0; // left split (modes/place actors)
};

class FDockspaceWidget
{
public:
    FDockspaceWidget();
    ~FDockspaceWidget();

    void Draw();

private:
    FDelegateHandle ImGuiDelegateHandle;
	FLayoutIds      LayoutIds;
};