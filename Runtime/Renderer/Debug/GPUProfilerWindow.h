#pragma once
#include "GPUProfiler.h"

#include "Application/IWindow.h"

#include <imgui.h>

class CGPUProfilerWindow : public IWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CGPUProfilerWindow> Make();

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    CGPUProfilerWindow() = default;
    ~CGPUProfilerWindow() = default;

    /* Draw the profiler window */
    void DrawWindow();

    /* Draw the GPU data */
    void DrawGPUData(float Width);

    /* Stores tables here to avoid allocating memory every frame */
    GPUProfileSamplesTable Samples;
};