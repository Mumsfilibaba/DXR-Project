#pragma once
#include "Core/Containers/Array.h"
#include "Core/Misc/ProfilerReport.h"
#include "Core/Misc/ProfilerTypes.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"

class FButton;
class FComboBox;
class FExpander;
class FHistogram;
class FHorizontalBox;
class FProfilerTimeline;
class FPropertyTable;
class FSearchBox;
class FTextBlock;
class FToolBar;
class FToolTipHost;
class FToolBarButton;
class FTreeView;
struct FTreeItem;
class FVerticalBox;
struct FProfilerTimelineLane;

class ENGINE_API FEditorProfilerPanel final : public FEditorPanel
{
public:
    FEditorProfilerPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorProfilerPanel();

    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void OnVisibilityChanged(bool bInIsVisible) override final;

    void SelectGpuScopeByName(const String& Name);

    NODISCARD int32 GetShownCpuFrameIndex() const
    {
        return ShownFrameNumber;
    }

    NODISCARD bool IsLiveView() const
    {
        return bFollowLatestFrame && !bIsFrozen;
    }

private:
    enum class EProfilerMode : uint8
    {
        Frames,
        Boot,
    };

    enum class ETargetDomain : uint8
    {
        Cpu,
        Gpu,
    };

    struct FScopeRef
    {
        int32 LaneIndex     = -1;
        int32 IntervalIndex = -1;
        bool  bIsGpu        = false;
    };

    struct FCallTreeNode
    {
        const CHAR*           Name = nullptr;
        int32                 ParentNode = -1;
        int32                 Calls = 0;
        uint64                InclusiveNanoseconds = 0;
        uint64                ExclusiveNanoseconds = 0;
        int32                 LongestInterval = -1;
        uint64                LongestInclusive = 0;
        TSharedPtr<FTreeItem> Item;
    };

    NODISCARD static TSharedPtr<FTextBlock> CreateValueText(const String& Text);
    NODISCARD static String ResolveThreadName(void* ThreadHandle, int32 ThreadIndex);
    NODISCARD static String FormatScopeColumns(int32 Calls, uint64 InclusiveNanoseconds, uint64 ExclusiveNanoseconds, double LanePercent);
    NODISCARD static String FormatTargetColumns(double CallsPerFrame, double InclusiveMilliseconds, double ExclusiveMilliseconds, double BudgetPercent);
    
    NODISCARD TSharedPtr<FToolBar> BuildToolBar();
    NODISCARD TArray<FProfilerOptimizationTarget> CollectGpuOptimizationTargets() const;
    NODISCARD const FProfilerFrame* ResolveSelectedFrame() const;
    NODISCARD int32 FindStoredIndexForFrame(int32 FrameNumber) const;

    void SetTargetDomain(ETargetDomain InDomain);
    void SetProfilingEnabled(bool bEnabled);
    void SetCaptureNativeStacks(bool bEnabled);
    void SetPipelineStatisticsEnabled(bool bEnabled);
    void SetCollectOptimizationTargets(bool bEnabled);
    void SetMode(EProfilerMode Mode);
    void SetFollowLatestFrame(bool bFollow);
    void SetFrozen(bool bFrozen);
    void PinCurrentFrame();
    void JumpToFrame(int32 FrameNumber);
    void ApplyProfilerState();
    void RefreshHeader();
    void RefreshHistograms();
    void RefreshFrameSelection();
    void RefreshDetails();
    void RefreshPipelineStatistics();
    void RefreshOptimizationTargets();
    void RefreshWorstFrames();
    void RebuildDetailSections();
    void FillTimeline();
    void RebuildHierarchy();
    void RebuildSelectionDetails();
    void RebuildCallers();
    void RebuildNativeStack();
    void ApplySearchToHierarchy();
    void SelectTreeItemForScope(const FScopeRef& Scope);
    void OnSearchTextChanged(const String& Text);

    NODISCARD TSharedPtr<FTreeItem> BuildCallTree(
        int32                        LaneIndex,
        const String&                LaneLabel,
        const TArray<const CHAR*>&   Names,
        const TArray<int32>&         Parents,
        const TArray<uint64>&        Inclusive,
        const TArray<uint64>&        Exclusive,
        bool                         bIsGpu);

    NODISCARD TSharedPtr<FVisualElement> BuildTimelineBarContextMenu(int32 LaneIndex, int32 BarIndex);
    NODISCARD TSharedPtr<FVisualElement> BuildScopeContextMenu(const TSharedPtr<FTreeItem>& Item);
    NODISCARD TSharedPtr<FVisualElement> BuildGpuPassContextMenu(const String& PassName);

    void OnTreeSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection);
    void OnTreeExpansionChanged(const TSharedPtr<FTreeItem>& Item, bool bIsExpanded);
    void OnTargetSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection);
    void OnTimelineBarSelected(int32 LaneIndex, int32 BarIndex);
    void ShowGpuPassInRenderGraph(const String& PassName);
    void WriteCapture();
    void BuildPipelineStatisticsRows();
    void BindPipelineStatistics(const FRHIPipelineStatistics* Statistics);

    TSharedPtr<FTextBlock>                SummaryText;
    TSharedPtr<FToolBar>                  ToolBar;
    TSharedPtr<FToolBarButton>            FramesToggle;
    TSharedPtr<FToolBarButton>            BootToggle;
    TSharedPtr<FToolBarButton>            LiveToggle;
    TSharedPtr<FToolBarButton>            FreezeToggle;
    TSharedPtr<FSearchBox>                SearchBox;
    TSharedPtr<FHistogram>                CpuHistogram;
    TSharedPtr<FHistogram>                GpuHistogram;
    TSharedPtr<FHorizontalBox>            WorstFramesRow;
    TArray<TSharedPtr<FButton>>           WorstFrameButtons;
    TArray<TSharedPtr<FToolTipHost>>      WorstFrameToolTips;
    TArray<int32>                         WorstFrameNumbers;
    TSharedPtr<FProfilerTimeline>         Timeline;
    TSharedPtr<FTreeView>                 HierarchyTree;
    TSharedPtr<FTreeView>                 TargetsTree;
    TSharedPtr<FExpander>                 CallersExpander;
    TSharedPtr<FExpander>                 NativeCallstacksExpander;
    TSharedPtr<FExpander>                 OptimizationTargetsExpander;
    TSharedPtr<FExpander>                 PipelineStatisticsExpander;
    TSharedPtr<FVerticalBox>              DetailSections;
    TSharedPtr<FComboBox>                 TargetDomainCombo;
    TArray<String>                        TargetScopeNames;
    TSharedPtr<FPropertyTable>            CallersTable;
    TSharedPtr<FPropertyTable>            NativeStackTable;
    TSharedPtr<FPropertyTable>            PipelineStatisticsTable;
    TSharedPtr<FTextBlock>                PipelineStatisticsContext;
    TArray<TSharedPtr<FTextBlock>>        PipelineStatisticsValues;
    FProfilerFrame                        ShownFrame;
    FProfilerFrame                        PinnedFrameSnapshot;
    TArray<FGPUProfilerInterval>          GpuIntervals;
    TArray<TSharedPtr<FTreeItem>>         HierarchyRoots;
    TArray<FScopeRef>                     HierarchyRefs;
    TArray<TArray<TSharedPtr<FTreeItem>>> LaneIntervalItems;
    FScopeRef                             Selection;
    String                                SearchQuery;
    String                                GpuStatusText;
    EProfilerMode                         Mode;
    ETargetDomain                         TargetDomain;
    float                                 TimeSinceDetailRefresh;
    float                                 TimeSinceAggregateRefresh;
    int32                                 LastIngestedCpuFrameIndex;
    int32                                 LastIngestedGpuCpuFrameIndex;
    int32                                 ShownFrameNumber;
    int32                                 PinnedFrameNumber;
    int32                                 AppliedHistogramSample;
    int32                                 AppliedGpuHistogramSample;
    float                                 ShownGpuMilliseconds;
    bool                                  bFollowLatestFrame;
    bool                                  bFollowLatestFrameBeforeFreeze;
    bool                                  bIsFrozen;
    bool                                  bHasIngestedGpuFrame;
    bool                                  bIsProfilingRequested;
    bool                                  bCaptureNativeStacks;
    bool                                  bCollectOptimizationTargets;
    bool                                  bDetailsAreDirty;
    bool                                  bHasShownGpuFrame;
    bool                                  bHasPinnedFrameSnapshot;
};
