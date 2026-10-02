#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ProfilerReport.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FExpander;
class FHistogram;
class FPropertyTable;
class FTextBlock;
class FToolBar;
class FVerticalBox;

class ENGINE_API FEditorFrameProfilerPanel final : public FEditorPanel
{
public:
    FEditorFrameProfilerPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorFrameProfilerPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void OnVisibilityChanged(bool bInIsVisible) override final;

private:
    struct FThreadSection
    {
        TSharedPtr<FExpander>                Section;
        TSharedPtr<FPropertyTable>           Table;
        TMap<String, TSharedPtr<FTextBlock>> Values;
    };

    NODISCARD static String ResolveThreadName(const FProfilerThreadAggregate& ThreadInfo, int32 ThreadIndex);
    NODISCARD static TSharedPtr<FTextBlock> CreateValueText(const String& Text);

    NODISCARD TSharedPtr<FToolBar> BuildToolBar();

    void SetProfilingEnabled(bool bEnabled);
    void ApplyProfilerState();
    void RefreshFrameTime();
    void RefreshThreads();
    void RebuildThreadRows(FThreadSection& ThreadSection, const FProfilerThreadAggregate& ThreadInfo);

    TSharedPtr<FToolBar>             ToolBar;
    TSharedPtr<FHistogram>           FrameTimeHistogram;
    TSharedPtr<FVerticalBox>         ThreadsColumn;
    TArray<FThreadSection>           ThreadSections;
    TArray<FProfilerThreadAggregate> ThreadInfos;
    int32                            LastIngestedFrameIndex;
    bool                             bIsProfilingRequested;
};
