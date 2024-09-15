#pragma once
#include "Core/Misc/FrameProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FFrameProfilerWidget : public IImGuiWidget
{
public:
    virtual void Draw() override final;

    void DrawFPS();
    void DrawWindow();
    void DrawCPUData(float Width);

private:
    TArray<FFrameProfilerThreadInfo> ThreadInfos;
};
