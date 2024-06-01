#pragma once
#include "Application/Widget.h"
#include "Core/Misc/FrameProfiler.h"

class FFrameProfilerWidget : public FWidget
{
public:
    virtual void Paint() override final;

    void DrawFPS();
    void DrawWindow();
    void DrawCPUData(float Width);

private:
    TArray<FFrameProfilerThreadInfo> ThreadInfos;
};
