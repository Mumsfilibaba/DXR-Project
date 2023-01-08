#pragma once
#include "GPUProfiler.h"

#include "Application/Widget.h"

#include <imgui.h>

class FGPUProfilerWindow 
    : public FWidget
{
    INTERFACE_GENERATE_BODY();

    FGPUProfilerWindow()  = default;
    ~FGPUProfilerWindow() = default;

public:
    static TSharedRef<FGPUProfilerWindow> Create();

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief - Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:
     /** @brief - Draw the profiler window */
    void DrawWindow();

     /** @brief - Draw the GPU data */
    void DrawGPUData(float Width);

     /** @brief - Stores tables here to avoid allocating memory every frame */
    GPUProfileSamplesTable Samples;
};
