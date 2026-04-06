#pragma once
#include "Core/Delegates/Delegate.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorGPUProfilerWidget
{
public:
    FEditorGPUProfilerWidget();
    ~FEditorGPUProfilerWidget();

    void Draw();
    void DrawWindow();
    void DrawGPUData();
    void DrawPipelineStatistics();

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    GPUProfileSamplesMap Samples;
    FDelegateHandle      ImGuiDelegateHandle;
    bool                 bVisible;
};
