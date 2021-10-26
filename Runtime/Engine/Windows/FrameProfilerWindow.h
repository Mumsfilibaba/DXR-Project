#pragma once
#include "Core/Debug/FrameProfiler.h"
#include "Core/Application/UI/IUIWindow.h"

class CFrameProfilerWindow : public IUIWindow
{
public:
    static TSharedRef<CFrameProfilerWindow> Make();

    /* Initializes the panel. The context handle should be set if the global context is not yet, this ensures that panels can be created from different DLLs*/
    virtual void InitContext( UIContextHandle ContextHandle ) override final;

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
    void DrawCPUData( float Width );

    /* Draw the GPU data */
    void DrawGPUData( float Width );

    /* Stores tables here to avoid allocating memory every frame */
    ProfileSamplesTable    CPUSamples;
    GPUProfileSamplesTable GPUSamples;
};