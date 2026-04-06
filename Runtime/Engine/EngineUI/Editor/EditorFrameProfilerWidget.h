#pragma once
#include "Core/Misc/FrameProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorFrameProfilerWidget
{
public:
    FEditorFrameProfilerWidget();
    ~FEditorFrameProfilerWidget();

    void Draw();
    void DrawWindow();
    void DrawCPUData(float Width);

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:
    TArray<FFrameProfilerThreadInfo> ThreadInfos;
    FDelegateHandle                  ImGuiDelegateHandle;
    bool                             bVisible;
};
