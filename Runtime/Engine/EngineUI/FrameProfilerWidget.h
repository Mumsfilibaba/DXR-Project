#pragma once
#include "Core/Misc/FrameProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FFrameProfilerWidget
{
public:
    FFrameProfilerWidget();
    ~FFrameProfilerWidget();

    void Draw();
    void DrawFPS();
    void DrawWindow();
    void DrawCPUData(float Width);

private:
    TArray<FFrameProfilerThreadInfo> ThreadInfos;
    FDelegateHandle                  ImGuiDelegateHandle;
};
