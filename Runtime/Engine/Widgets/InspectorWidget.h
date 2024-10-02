#pragma once
#include "Core/Misc/FrameProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FInspectorWidget
{
public:
    FInspectorWidget();
    ~FInspectorWidget();

    void Draw();
    void DrawSceneInfo();

private:
    FDelegateHandle ImGuiDelegateHandle;
};