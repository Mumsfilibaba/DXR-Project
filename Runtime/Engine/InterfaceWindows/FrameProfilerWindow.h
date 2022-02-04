#pragma once
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Application/IWindow.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CFrameProfilerWindow

class CFrameProfilerWindow : public IWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CFrameProfilerWindow> Make();

    virtual void Tick() override final;

    virtual bool IsTickable() override final;

private:

    CFrameProfilerWindow() = default;
    ~CFrameProfilerWindow() = default;

    /* Draw a simple FPS counter */
    void DrawFPS();

    /* Draw the profiler window */
    void DrawWindow();

    /* Draw the CPU data */
    void DrawCPUData(float Width);

    ProfileSamplesTable Samples;
};