#pragma once
#include "Application/Window.h"

#include "Core/Misc/FrameProfiler.h"

#include <imgui.h>

class FFrameProfilerWindow 
    : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:
    static TSharedRef<FFrameProfilerWindow> Make();

    virtual void Tick() override final;

    virtual bool IsTickable() override final;

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
