#pragma once
#include "Core/Containers/Array.h"
#include "Core/Misc/ProfilerReport.h"
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
    TArray<FProfilerThreadAggregate> ThreadInfos;
    FDelegateHandle                  ImGuiDelegateHandle;
    bool                             bVisible;
};
