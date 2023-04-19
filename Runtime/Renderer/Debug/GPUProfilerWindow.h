#pragma once
#include "GPUProfiler.h"
#include "Application/Widget.h"

class FGPUProfilerWindow : public FWidget
{
    DECLARE_WIDGET(FGPUProfilerWindow, FWidget);

public:
    FINITIALIZER_START(FGPUProfilerWindow)
    FINITIALIZER_END();

    void Initialize(const FInitializer& Initilizer)
    {
    }

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Paint(const FRectangle& AssignedBounds) override final;

    void DrawWindow();
    void DrawGPUData(float Width);

private:
    GPUProfileSamplesTable Samples;
};
