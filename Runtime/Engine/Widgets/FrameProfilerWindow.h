#pragma once
#include "Application/Widget.h"
#include "Core/Misc/FrameProfiler.h"

class FFrameProfilerWindow 
    : public FWidget
{
public:
    static TSharedRef<FFrameProfilerWindow> Create();

    virtual void Tick() override final;

    virtual bool ShouldTick() override final;

private:
    FFrameProfilerWindow() = default;
    ~FFrameProfilerWindow() = default;

     /** @brief - Draw a simple FPS counter */
    void DrawFPS();

     /** @brief - Draw the profiler window */
    void DrawWindow();

     /** @brief - Draw the CPU data */
    void DrawCPUData(float Width);

    ProfileSamplesTable Samples;
};
