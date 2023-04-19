#pragma once
#include "Application/Widget.h"
#include "Core/Misc/FrameProfiler.h"

class FFrameProfilerWindow : public FWidget
{
    DECLARE_WIDGET(FFrameProfilerWindow, FWidget);

public:
    FINITIALIZER_START(FFrameProfilerWindow)
    FINITIALIZER_END();

    void Initialize(const FInitializer& Initializer)
    {
    }

    virtual void Paint(const FRectangle& AssignedBounds) override final;

     /** @brief - Draw a simple FPS counter */
    void DrawFPS();

     /** @brief - Draw the profiler window */
    void DrawWindow();

     /** @brief - Draw the CPU data */
    void DrawCPUData(float Width);

private:
    ProfileSamplesTable Samples;
};
