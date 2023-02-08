#pragma once
#include "Application/Widget.h"
#include "Core/Misc/FrameProfiler.h"

class FFrameProfilerWindow
    : public FWidget
{
public:
    virtual void OnDraw() override final;

     /** @brief - Draw a simple FPS counter */
    void DrawFPS();

     /** @brief - Draw the profiler window */
    void DrawWindow();

     /** @brief - Draw the CPU data */
    void DrawCPUData(float Width);

private:
    ProfileSamplesTable Samples;
};
