#pragma once
#include "Canvas/Window.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CFrameProfilerWindow

class CFrameProfilerWindow : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CFrameProfilerWindow> Make();

    virtual void Tick() override final;

    virtual bool IsTickable() override final;

private:

    CFrameProfilerWindow() = default;
    ~CFrameProfilerWindow() = default;

     /** @brief: Draw a simple FPS counter */
    void DrawFPS();

     /** @brief: Draw the profiler window */
    void DrawWindow();

     /** @brief: Draw the CPU data */
    void DrawCPUData(float Width);

    ProfileSamplesTable Samples;
};
