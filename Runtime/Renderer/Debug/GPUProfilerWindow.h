#pragma once
#include "GPUProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FGPUProfilerWindow : public IImGuiWidget
{
public:

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Draw() override final;

    void DrawWindow();
    void DrawGPUData(float Width);

private:
    GPUProfileSamplesMap Samples;
};
