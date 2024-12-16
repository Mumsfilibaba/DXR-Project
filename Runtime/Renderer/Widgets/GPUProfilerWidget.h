#pragma once
#include "Renderer/Performance/GPUProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FGPUProfilerWidget
{
public:
    FGPUProfilerWidget();
    ~FGPUProfilerWidget();

    void Draw();
    void DrawWindow();
    void DrawGPUData(float Width);

private:
    GPUProfileSamplesMap Samples;
    FDelegateHandle      ImGuiDelegateHandle;
};
