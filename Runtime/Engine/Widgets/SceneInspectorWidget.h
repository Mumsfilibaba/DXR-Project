#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FSceneInspectorWidget
{
public:
    FSceneInspectorWidget();
    ~FSceneInspectorWidget();

    void Draw();
    void DrawSceneInfo();

private:
    FDelegateHandle ImGuiDelegateHandle;
};