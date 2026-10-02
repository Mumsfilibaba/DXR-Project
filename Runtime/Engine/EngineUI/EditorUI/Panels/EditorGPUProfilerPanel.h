#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FHistogram;
class FPropertyTable;
class FTextBlock;
class FToolBar;

class ENGINE_API FEditorGPUProfilerPanel final : public FEditorPanel
{
public:
    FEditorGPUProfilerPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorGPUProfilerPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void OnVisibilityChanged(bool bInIsVisible) override final;

private:
    NODISCARD static TSharedPtr<FTextBlock> CreateValueText(const String& Text);

    NODISCARD TSharedPtr<FToolBar> BuildToolBar();

    void SetProfilingEnabled(bool bEnabled);
    void ApplyProfilerState();
    void RefreshFrameTime(IGPUProfiler& Profiler);
    void RefreshPasses(IGPUProfiler& Profiler);
    void RefreshPipelineStatistics(IGPUProfiler& Profiler);
    void BuildPipelineStatisticsRows();
    void RebuildPassTable();

    TSharedPtr<FToolBar>                 ToolBar;
    TSharedPtr<FHistogram>               FrameTimeHistogram;
    TSharedPtr<FPropertyTable>           PassTable;
    TSharedPtr<FPropertyTable>           PipelineStatisticsTable;
    TMap<String, TSharedPtr<FTextBlock>> PassValues;
    TArray<TSharedPtr<FTextBlock>>       PipelineStatisticsValues;
    int32                                LastIngestedCpuFrameIndex;
    bool                                 bIsProfilingRequested;
};
