#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FSceneRenderer;

class FRendererInfoWidget
{
public:
    FRendererInfoWidget(FSceneRenderer* InRenderer);
    ~FRendererInfoWidget();
     
    void Draw();

private:
    FSceneRenderer* Renderer;
    FDelegateHandle ImGuiDelegateHandle;
};
