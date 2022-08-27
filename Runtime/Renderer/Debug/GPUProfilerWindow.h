#pragma once
#include "GPUProfiler.h"

#include "Application/Window.h"

#include <imgui.h>

class FGPUProfilerWindow 
    : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:
    static TSharedRef<FGPUProfilerWindow> Make();

     /** @brief: Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief: Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:
    FGPUProfilerWindow()  = default;
    ~FGPUProfilerWindow() = default;

     /** @brief: Draw the profiler window */
    void DrawWindow();

     /** @brief: Draw the GPU data */
    void DrawGPUData(float Width);

     /** @brief: Stores tables here to avoid allocating memory every frame */
    GPUProfileSamplesTable Samples;
};
