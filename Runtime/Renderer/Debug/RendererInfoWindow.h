#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FSceneRenderer;

class FRendererInfoWindow : public IImGuiWidget
{
public:
    FRendererInfoWindow(FSceneRenderer* InRenderer);
    ~FRendererInfoWindow();
     
    /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Draw() override final;

private:
    FSceneRenderer* Renderer;
};
