#pragma once
#include "GPUProfiler.h"
#include "Application/Widget.h"

class FGPUProfilerWindow 
    : public FWidget
{
public:

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void OnDraw() override final;

    void DrawWindow();
    void DrawGPUData(float Width);

private:
    GPUProfileSamplesTable Samples;
};
