#pragma once
#include "Application/Widget.h"
#include "Core/Containers/SharedRef.h"

class FSceneRenderer;

class FRendererInfoWindow : public FWidget
{
public:
    FRendererInfoWindow(FSceneRenderer* InRenderer)
        : Renderer(InRenderer)
    {
    }
     
    /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Paint() override final;

private:
    FSceneRenderer* Renderer;
};
