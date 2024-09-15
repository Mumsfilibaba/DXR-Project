#pragma once
#include "Core/Misc/FrameProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FInspectorWidget : public IImGuiWidget
{
public:
    virtual void Draw() override final;

private:

    // Draws the scene info, should only be called from tick
    void DrawSceneInfo();
};