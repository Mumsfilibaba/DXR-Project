#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FSceneRenderer;

class FRendererInfoWindow
{
public:
    FRendererInfoWindow(FSceneRenderer* InRenderer);
    ~FRendererInfoWindow();
     
    void Draw();

private:
    FSceneRenderer* Renderer;
    FDelegateHandle ImGuiDelegateHandle;
};
