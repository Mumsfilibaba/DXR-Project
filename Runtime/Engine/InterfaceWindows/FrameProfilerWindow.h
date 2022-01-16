#pragma once
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Interface/IInterfaceWindow.h"

#include <imgui.h>

class CFrameProfilerWindow : public IInterfaceWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CFrameProfilerWindow> Make();

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
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

    /* Stores tables here to avoid allocating memory every frame */
    ProfileSamplesTable Samples;
};