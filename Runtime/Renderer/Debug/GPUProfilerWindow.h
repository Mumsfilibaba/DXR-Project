#pragma once
#include "GPUProfiler.h"
#include "Application/Widget.h"

class FGPUProfilerWindow 
    : public FWidget
{
    FGPUProfilerWindow()  = default;
    ~FGPUProfilerWindow() = default;

public:
    static TSharedRef<FGPUProfilerWindow> Create();

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief - Returns true if the panel should be updated this frame */
    virtual bool ShouldTick() override final;

private:
    void DrawWindow();
    void DrawGPUData(float Width);

    GPUProfileSamplesTable Samples;
};
