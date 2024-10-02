#pragma once
#include "GPUProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FGPUProfilerWindow
{
public:
    FGPUProfilerWindow();
    ~FGPUProfilerWindow();

    void Draw();
    void DrawWindow();
    void DrawGPUData(float Width);

private:
    GPUProfileSamplesMap Samples;
    FDelegateHandle      ImGuiDelegateHandle;
};
