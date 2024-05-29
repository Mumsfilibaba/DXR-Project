#pragma once
#include "Application/Widget.h"
#include "Core/Misc/FrameProfiler.h"

class FInspectorWidget : public FWidget
{
public:
    virtual void Paint() override final;

private:

    // Draws the scene info, should only be called from tick
    void DrawSceneInfo();
};