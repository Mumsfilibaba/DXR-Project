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

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    TArray<FFrameProfilerThreadInfo> ThreadInfos;
    FDelegateHandle                  ImGuiDelegateHandle;
    bool                             bVisible;
};
