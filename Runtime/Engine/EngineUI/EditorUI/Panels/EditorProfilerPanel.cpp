#include "Engine/EngineUI/EditorUI/Panels/EditorProfilerPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorRenderGraphPanel.h"
#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/EditorShell.h"
#include "Engine/EngineUI/EditorUI/EditorPanelRegistry.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/Histogram.h"
#include "Application/Elements/ProfilerTimeline.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Elements/TreeView.h"
#include "Application/Menus/ComboBox.h"
#include "Application/Menus/Menu.h"
#include "Core/Containers/Map.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Misc/BootProfiler.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/ProfilerReport.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Templates/CString.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Time/Time.h"
#include "RendererCore/Interfaces/IRendererModule.h"

static constexpr int32 PROFILER_TYPE_COLUMN_WIDTH        = 300;
static constexpr int32 PROFILER_DEFAULT_EXPAND_DEPTH     = 2;
static constexpr int32 PROFILER_MAX_OPTIMIZATION_TARGETS = 20;
static constexpr float LIVE_REFRESH_INTERVAL             = 0.125f;

struct FPipelineStatisticsCounter
{
    const CHAR*                      Label;
    uint64 FRHIPipelineStatistics::* Member;
};

static const FPipelineStatisticsCounter GPipelineStatisticsCounters[] =
{
    { "IA Vertices",          &FRHIPipelineStatistics::IAVertices    },
    { "IA Primitives",        &FRHIPipelineStatistics::IAPrimitives  },
    { "VS Invocations",       &FRHIPipelineStatistics::VSInvocations },
    { "HS Invocations",       &FRHIPipelineStatistics::HSInvocations },
    { "DS Invocations",       &FRHIPipelineStatistics::DSInvocations },
    { "GS Invocations",       &FRHIPipelineStatistics::GSInvocations },
    { "GS Primitives",        &FRHIPipelineStatistics::GSPrimitives  },
    { "Clipping Invocations", &FRHIPipelineStatistics::CInvocations  },
    { "Clipping Primitives",  &FRHIPipelineStatistics::CPrimitives   },
    { "PS Invocations",       &FRHIPipelineStatistics::PSInvocations },
    { "CS Invocations",       &FRHIPipelineStatistics::CSInvocations },
    { "AS Invocations",       &FRHIPipelineStatistics::ASInvocations },
    { "MS Invocations",       &FRHIPipelineStatistics::MSInvocations },
    { "MS Primitives",        &FRHIPipelineStatistics::MSPrimitives  },
};

static IGPUProfiler* GetGPUProfiler()
{
    IRendererModule* RendererModule = IRendererModule::Get();
    return RendererModule ? &RendererModule->GetGPUProfiler() : nullptr;
}

static bool AreScopeNamesEqual(const CHAR* Lhs, const CHAR* Rhs)
{
    if (Lhs == Rhs)
    {
        return true;
    }

    return Lhs && Rhs && CString::Strcmp(Lhs, Rhs) == 0;
}

static bool NameContainsIgnoreCase(const CHAR* Name, const String& Query)
{
    if (Query.IsEmpty())
    {
        return true;
    }

    return Name && CString::Stristr(Name, *Query) != nullptr;
}

static float ToMilliseconds(uint64 Nanoseconds)
{
    return static_cast<float>(Time::ToMilliseconds(static_cast<double>(Nanoseconds)));
}

static FRHIPipelineStatistics SumPipelineStatistics(const TArray<FGPUProfilerInterval>& Intervals)
{
    FRHIPipelineStatistics Total = {};
    for (const FGPUProfilerInterval& Interval : Intervals)
    {
        if (!Interval.bHasPipelineStats)
        {
            continue;
        }

        Total.IAVertices    += Interval.PipelineStats.IAVertices;
        Total.IAPrimitives  += Interval.PipelineStats.IAPrimitives;
        Total.VSInvocations += Interval.PipelineStats.VSInvocations;
        Total.GSInvocations += Interval.PipelineStats.GSInvocations;
        Total.GSPrimitives  += Interval.PipelineStats.GSPrimitives;
        Total.CInvocations  += Interval.PipelineStats.CInvocations;
        Total.CPrimitives   += Interval.PipelineStats.CPrimitives;
        Total.PSInvocations += Interval.PipelineStats.PSInvocations;
        Total.HSInvocations += Interval.PipelineStats.HSInvocations;
        Total.DSInvocations += Interval.PipelineStats.DSInvocations;
        Total.CSInvocations += Interval.PipelineStats.CSInvocations;
        Total.ASInvocations += Interval.PipelineStats.ASInvocations;
        Total.MSInvocations += Interval.PipelineStats.MSInvocations;
        Total.MSPrimitives  += Interval.PipelineStats.MSPrimitives;
    }

    return Total;
}

static String BuildChromeGpuEvents()
{
    IGPUProfiler* Gpu = GetGPUProfiler();
    if (!Gpu)
    {
        return String();
    }

    String Events;
    Events.Append("{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":1,\"tid\":0,\"args\":{\"name\":\"GPU\"}}");

    FProfilerGpuFrame GpuFrame;

    const int32 Count = Gpu->GetStoredFrameCount();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        if (!Gpu->GetStoredFrame(Index, GpuFrame))
        {
            continue;
        }

        double FrameStartUs = 0.0;
        if (!FProfilerReport::TryGetChromeFrameTimestampUs(GpuFrame.CpuFrameIndex, 0, FrameStartUs))
        {
            continue;
        }

        for (const FGPUProfilerInterval& Interval : GpuFrame.Intervals)
        {
            const double TimestampUs = FrameStartUs + Time::ToMicroseconds(static_cast<double>(Interval.StartNanoseconds));

            const CHAR* Name = Interval.Name ? Interval.Name : "<unnamed>";
            if (Interval.bInstant || Interval.InclusiveNanoseconds == 0)
            {
                FProfilerReport::AppendChromeInstantEvent(Events, Name, "gpu", TimestampUs, 1, 0);
            }
            else
            {
                const double DurationUs = Time::ToMicroseconds(static_cast<double>(Interval.InclusiveNanoseconds));
                FProfilerReport::AppendChromeCompleteEvent(Events, Name, "gpu", TimestampUs, DurationUs, 1, 0);
            }
        }
    }

    return Events;
}

FEditorProfilerPanel::FEditorProfilerPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "Profiler", "Profiler")
    , Mode(EProfilerMode::Frames)
    , TargetDomain(ETargetDomain::Cpu)
    , TimeSinceDetailRefresh(0.0f)
    , TimeSinceAggregateRefresh(0.0f)
    , LastIngestedCpuFrameIndex(-1)
    , LastIngestedGpuCpuFrameIndex(-1)
    , ShownFrameNumber(-1)
    , PinnedFrameNumber(-1)
    , AppliedHistogramSample(-1)
    , AppliedGpuHistogramSample(-1)
    , ShownGpuMilliseconds(0.0f)
    , bFollowLatestFrame(true)
    , bFollowLatestFrameBeforeFreeze(true)
    , bIsFrozen(false)
    , bHasIngestedGpuFrame(false)
    , bIsProfilingRequested(true)
    , bCaptureNativeStacks(false)
    , bCollectOptimizationTargets(true)
    , bDetailsAreDirty(true)
    , bHasShownGpuFrame(false)
{
}

FEditorProfilerPanel::~FEditorProfilerPanel() = default;

bool FEditorProfilerPanel::Initialize()
{
    ToolBar = BuildToolBar();
    if (!ToolBar)
    {
        return false;
    }

    TSharedPtr<FVisualElement> HeaderCard = FEditorStyle::MakeHeaderCard("Profiler", "Waiting for samples", &SummaryText);

    SearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search scopes",
        FOnSearchTextChanged::CreateRaw(this, &FEditorProfilerPanel::OnSearchTextChanged)));

    FHistogram::FDesc CpuDesc;
    CpuDesc.Capacity        = NUM_LIVE_PROFILER_FRAMES;
    CpuDesc.Font            = FEditorStyle::GetFonts().Body;
    CpuDesc.Label           = "CPU (ms)";
    CpuDesc.PreferredHeight = 48;
    CpuDesc.DrawMode        = EHistogramDrawMode::Line;
    CpuDesc.LineThickness   = 2.0f;
    CpuDesc.BarColor        = FFloatColor(0.28f, 0.76f, 0.52f, 1.0f);
    CpuHistogram = FHistogram::Create(CpuDesc);

    FHistogram::FDesc GpuDesc = CpuDesc;
    GpuDesc.Label    = "GPU (ms)";
    GpuDesc.BarColor = FFloatColor(0.34f, 0.56f, 0.94f, 1.0f);
    GpuHistogram     = FHistogram::Create(GpuDesc);

    WorstFramesRow = FHorizontalBox::Create();

    FProfilerTimeline::FDesc TimelineDesc;
    TimelineDesc.Font                = FEditorStyle::GetFonts().Body;
    TimelineDesc.PreferredHeight     = 180;
    TimelineDesc.OnBarSelected       = FOnProfilerBarSelected::CreateRaw(this, &FEditorProfilerPanel::OnTimelineBarSelected);
    TimelineDesc.OnGetBarContextMenu = FOnGetProfilerBarContextMenu::CreateRaw( this, &FEditorProfilerPanel::BuildTimelineBarContextMenu);
    Timeline = FProfilerTimeline::Create(TimelineDesc);

    FTreeView::FDesc TreeDesc;
    TreeDesc.Font               = FEditorStyle::GetFonts().Body;
    TreeDesc.RowHeight          = FEditorStyle::RowHeight;
    TreeDesc.TypeColumnWidth    = PROFILER_TYPE_COLUMN_WIDTH;
    TreeDesc.LabelColumnHeader  = "Scope";
    TreeDesc.TypeColumnHeader   = "Calls      Incl ms      Excl ms    % lane";
    TreeDesc.LabelColumnToolTip = "The frame's timing scopes, nested the way they ran.\n"
                                  "One lane per thread, plus a GPU lane for the passes.";

    TreeDesc.TypeColumnToolTips = 
    {
        String("How many times the scope was entered this frame."),
        String("Time spent in the scope this frame, its children included."),
        String("Time spent in the scope itself this frame, children taken out."),
        String("The share of its lane's total that inclusive time is, so a scope\n"
               "on a worker thread is read against that thread rather than the frame."),
    };

    TreeDesc.bShowScrollBar           = false;
    TreeDesc.bAlternateRowColors      = true;
    TreeDesc.bAllowMultiSelect        = false;
    TreeDesc.bFilterMatchesTypeColumn = false;
    TreeDesc.OnSelectionChanged       = FOnTreeSelectionChanged::CreateRaw(this, &FEditorProfilerPanel::OnTreeSelectionChanged);
    TreeDesc.OnExpansionChanged       = FOnTreeItemExpansionChanged::CreateRaw(this, &FEditorProfilerPanel::OnTreeExpansionChanged);
    TreeDesc.OnGetContextMenu         = FOnGetTreeItemContextMenu::CreateRaw(this, &FEditorProfilerPanel::BuildScopeContextMenu);

    FEditorStyle::ApplyTreeViewArrows(TreeDesc);
    HierarchyTree = FTreeView::Create(TreeDesc);

    FTreeView::FDesc TargetsDesc = TreeDesc;
    TargetsDesc.LabelColumnHeader  = "Scope";
    TargetsDesc.TypeColumnHeader   = "Calls/f    Incl ms/f    Excl ms/f   % frame";
    TargetsDesc.LabelColumnToolTip = "Scopes ranked by self time across the stored frames, hottest first.\n"
                                     "The number ahead of each name is where it places in that ranking.\n"
                                     "The drop-down picks whether CPU scopes or GPU passes are ranked.";
    
    TargetsDesc.TypeColumnToolTips = 
    {
        String("How often the scope ran, averaged over the stored frames."),
        String("Average time in the scope, its children included."),
        String("Average time in the scope itself, children taken out."),
        String("The share of the captured frame time that the scope's own work took."),
    };

    TargetsDesc.OnSelectionChanged       = FOnTreeSelectionChanged::CreateRaw(this, &FEditorProfilerPanel::OnTargetSelectionChanged);
    TargetsDesc.OnExpansionChanged       = FOnTreeItemExpansionChanged();
    TargetsDesc.OnGetContextMenu         = FOnGetTreeItemContextMenu();
    TargetsTree = FTreeView::Create(TargetsDesc);

    TArray<String> TargetDomainOptions;
    TargetDomainOptions.Add(String("CPU scopes"));
    TargetDomainOptions.Add(String("GPU passes"));

    FComboBox::FDesc TargetDomainDesc;
    TargetDomainDesc.Options            = TargetDomainOptions;
    TargetDomainDesc.SelectedIndex      = static_cast<int32>(TargetDomain);
    TargetDomainDesc.Font               = FEditorStyle::GetFonts().Body;
    TargetDomainDesc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([this](int32 SelectedIndex)
    {
        SetTargetDomain(SelectedIndex == static_cast<int32>(ETargetDomain::Gpu) ? ETargetDomain::Gpu : ETargetDomain::Cpu);
    });

    TargetDomainCombo = FComboBox::Create(TargetDomainDesc);

    CallersTable            = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
    NativeStackTable        = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
    PipelineStatisticsTable = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());

    if (!CpuHistogram || !GpuHistogram || !Timeline || !HierarchyTree || !SearchBox || !TargetsTree || !TargetDomainCombo
        || !CallersTable || !NativeStackTable || !PipelineStatisticsTable || !WorstFramesRow)
    {
        return false;
    }

    BuildPipelineStatisticsRows();

    TSharedPtr<FVerticalBox> FrameGraphs = FVerticalBox::Create();
    FrameGraphs->AddSlot(CpuHistogram).SetPadding(FMargin(0, 0, 0, 4));
    FrameGraphs->AddSlot(WorstFramesRow).SetPadding(FMargin(0, 0, 0, 4));
    FrameGraphs->AddSlot(GpuHistogram);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FEditorStyle::MakeInnerFrame(FrameGraphs)).SetPadding(FEditorStyle::GetSectionStackSpacing());
    Column->AddSlot(FEditorStyle::MakeInnerFrame(Timeline)).SetPadding(FEditorStyle::GetSectionStackSpacing());
    Column->AddSlot(FEditorStyle::MakeInnerFrame(HierarchyTree)).SetPadding(FEditorStyle::GetSectionStackSpacing());

    TSharedPtr<FHorizontalBox> TargetDomainRow = FHorizontalBox::Create();
    TargetDomainRow->AddSlot(TargetDomainCombo).SetHorizontalAlignment(EHorizontalAlignment::Left);

    TSharedPtr<FVerticalBox> Targets = FVerticalBox::Create();
    Targets->AddSlot(TargetDomainRow).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Targets->AddSlot(TargetsTree);

    CallersExpander             = FExpander::Create(FEditorStyle::MakeExpanderDesc("Callers", CallersTable, true));
    NativeCallstacksExpander    = FExpander::Create(FEditorStyle::MakeExpanderDesc("Native Callstacks", NativeStackTable, true));
    OptimizationTargetsExpander = FExpander::Create(FEditorStyle::MakeExpanderDesc("Optimization Targets", Targets, true));
    PipelineStatisticsExpander  = FExpander::Create(FEditorStyle::MakeExpanderDesc("Pipeline Statistics", PipelineStatisticsTable, false));

    DetailSections = FVerticalBox::Create();
    RebuildDetailSections();

    Column->AddSlot(DetailSections);

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(Column);

    TSharedPtr<FVerticalBox> Root = FVerticalBox::Create();
    Root->AddSlot(HeaderCard).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Root->AddSlot(ToolBar).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Root->AddSlot(SearchBox).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Root->AddSlot(ScrollBox).SetFillCoefficient(1.0f);

    Content = Root;

    RefreshDetails();
    return true;
}

TSharedPtr<FToolBar> FEditorProfilerPanel::BuildToolBar()
{
    FToolBar::FDesc Desc;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.IconSize       = FEditorStyle::IconSize;
    Desc.bHasBackground = true;

    TSharedPtr<FToolBar> Bar = FToolBar::Create(Desc);
    if (!Bar)
    {
        return nullptr;
    }

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Enable").SetToolTipText("Records CPU TRACE_SCOPE and GPU timestamp queries"),
        ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetProfilingEnabled(State == ECheckBoxState::Checked);
        }));

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Native stacks").SetToolTipText("Walks the OS callstack when a CPU scope closes"),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetCaptureNativeStacks(State == ECheckBoxState::Checked);
        }));

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Pipeline Statistics").SetToolTipText("Counts the work each top level GPU pass put through the pipeline"),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetPipelineStatisticsEnabled(State == ECheckBoxState::Checked);
        }));

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Optimization Targets").SetToolTipText("Ranks the hottest scopes for the Optimization Targets section.\n"
                                                                                     "Every refresh walks each stored frame, so it can be left off."),
        ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetCollectOptimizationTargets(State == ECheckBoxState::Checked);
        }));

    Bar->AddSeparator();
    Bar->BeginGroup();

    FramesToggle = Bar->AddToggle(FToolBarItemDesc().SetLabel("Frames").SetToolTipText("Shows the frames the engine is running now"),
        ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            if (State == ECheckBoxState::Checked)
            {
                SetMode(EProfilerMode::Frames);
            }
        }));

    BootToggle = Bar->AddToggle(FToolBarItemDesc().SetLabel("Boot").SetToolTipText("Shows the frozen session recorded before the first frame"),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            if (State == ECheckBoxState::Checked)
            {
                SetMode(EProfilerMode::Boot);
            }
        }));

    Bar->EndGroup();
    Bar->AddSeparator();

    LiveToggle = Bar->AddToggle(FToolBarItemDesc().SetLabel("Live").SetToolTipText("Follows the newest frame. Clicking a frame in the strip pins the panel to it"),
        ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            const bool bFollow = State == ECheckBoxState::Checked;
            if (bFollow && bIsFrozen)
            {
                SetFrozen(false);
            }

            SetFollowLatestFrame(bFollow);
        }));

    FreezeToggle = Bar->AddToggle(FToolBarItemDesc().SetLabel("Freeze").SetToolTipText("Keeps recording, but the strip and the timeline stay on what is on screen."),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetFrozen(State == ECheckBoxState::Checked);
        }));

    Bar->AddButton(FToolBarItemDesc().SetLabel("Fit").SetToolTipText("Puts the timeline back over the whole frame\n"
                                                                    "Ctrl + wheel zooms, the wheel alone scrolls\n"
                                                                    "Middle or alt drag pans, right click fits"),
        FOnClicked::CreateLambda([this]()
        {
            Timeline->ResetView();
            bDetailsAreDirty = true;
        }));

    Bar->AddButton(FToolBarItemDesc().SetLabel("Reset"), FOnClicked::CreateLambda([this]()
    {
        FFrameProfiler::Get().Reset();
        if (IGPUProfiler* Profiler = GetGPUProfiler())
        {
            Profiler->Reset();
        }

        CpuHistogram->Clear();
        GpuHistogram->Clear();

        LastIngestedCpuFrameIndex    = -1;
        LastIngestedGpuCpuFrameIndex = -1;
        bHasIngestedGpuFrame         = false;
        AppliedHistogramSample       = -1;
        AppliedGpuHistogramSample    = -1;
        PinnedFrameNumber            = -1;
        ShownFrameNumber             = -1;
        Selection                    = FScopeRef();

        SetFrozen(false);
        SetFollowLatestFrame(true);
        RefreshDetails();
    }));

    Bar->AddButton(FToolBarItemDesc().SetLabel("Capture").SetToolTipText("Writes Profiling/ProfileCapture_*.txt and .json"),
        FOnClicked::CreateLambda([this]()
        {
            WriteCapture();
        }));

    return Bar;
}

void FEditorProfilerPanel::Release()
{
    bIsProfilingRequested = false;
    ApplyProfilerState();
    FEditorPanel::Release();
}

void FEditorProfilerPanel::OnVisibilityChanged(bool bInIsVisible)
{
    FEditorPanel::OnVisibilityChanged(bInIsVisible);
    ApplyProfilerState();
}

void FEditorProfilerPanel::SetProfilingEnabled(bool bEnabled)
{
    bIsProfilingRequested = bEnabled;
    ApplyProfilerState();
}

void FEditorProfilerPanel::SetCaptureNativeStacks(bool bEnabled)
{
    bCaptureNativeStacks = bEnabled;

    FFrameProfiler::Get().SetCaptureNativeStacks(bEnabled);
    FBootProfiler::Get().SetCaptureNativeStacks(bEnabled);

    RebuildDetailSections();
    bDetailsAreDirty = true;
}

void FEditorProfilerPanel::SetPipelineStatisticsEnabled(bool bEnabled)
{
    if (IGPUProfiler* Profiler = GetGPUProfiler())
    {
        if (bEnabled)
        {
            Profiler->EnablePipelineStatistics();
        }
        else
        {
            Profiler->DisablePipelineStatistics();
        }
    }

    RebuildDetailSections();
    RefreshPipelineStatistics();
}

void FEditorProfilerPanel::SetCollectOptimizationTargets(bool bEnabled)
{
    if (bCollectOptimizationTargets == bEnabled)
    {
        return;
    }

    bCollectOptimizationTargets = bEnabled;
    TimeSinceAggregateRefresh   = 0.0f;

    RebuildDetailSections();
    RefreshOptimizationTargets();
}

void FEditorProfilerPanel::SetTargetDomain(ETargetDomain InDomain)
{
    if (TargetDomain == InDomain)
    {
        return;
    }

    TargetDomain              = InDomain;
    TimeSinceAggregateRefresh = 0.0f;

    RefreshOptimizationTargets();
}

void FEditorProfilerPanel::RebuildDetailSections()
{
    if (!DetailSections)
    {
        return;
    }

    DetailSections->ClearSlots();
    DetailSections->AddSlot(CallersExpander).SetPadding(FEditorStyle::GetSectionStackSpacing());

    if (bCaptureNativeStacks)
    {
        DetailSections->AddSlot(NativeCallstacksExpander).SetPadding(FEditorStyle::GetSectionStackSpacing());
    }

    if (bCollectOptimizationTargets)
    {
        DetailSections->AddSlot(OptimizationTargetsExpander).SetPadding(FEditorStyle::GetSectionStackSpacing());
    }

    const IGPUProfiler* Profiler = GetGPUProfiler();
    if (Profiler && Profiler->IsPipelineStatisticsEnabled())
    {
        DetailSections->AddSlot(PipelineStatisticsExpander).SetPadding(FEditorStyle::GetSectionStackSpacing());
    }
}

void FEditorProfilerPanel::SetMode(EProfilerMode InMode)
{
    Mode = InMode;
    if (FramesToggle)
    {
        FramesToggle->SetCheckState(Mode == EProfilerMode::Frames ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }

    if (BootToggle)
    {
        BootToggle->SetCheckState(Mode == EProfilerMode::Boot ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }

    Selection = FScopeRef();
    Timeline->ResetView();
    RefreshDetails();
}

void FEditorProfilerPanel::SetFollowLatestFrame(bool bFollow)
{
    bFollowLatestFrame = bFollow;
    if (LiveToggle)
    {
        LiveToggle->SetCheckState(bFollow ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }

    bDetailsAreDirty = true;
}

void FEditorProfilerPanel::SetFrozen(bool bFrozen)
{
    if (bIsFrozen == bFrozen)
    {
        return;
    }

    if (bFrozen)
    {
        bFollowLatestFrameBeforeFreeze = bFollowLatestFrame;
        bIsFrozen = true;
        SetFollowLatestFrame(false);
    }
    else
    {
        bIsFrozen = false;
        SetFollowLatestFrame(bFollowLatestFrameBeforeFreeze);
    }

    if (FreezeToggle)
    {
        FreezeToggle->SetCheckState(bIsFrozen ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
}

void FEditorProfilerPanel::PinCurrentFrame()
{
    if (Mode == EProfilerMode::Frames && bFollowLatestFrame && ShownFrameNumber >= 0)
    {
        PinnedFrameNumber = ShownFrameNumber;
        SetFollowLatestFrame(false);
        bDetailsAreDirty = true;
    }
}

void FEditorProfilerPanel::ApplyProfilerState()
{
    if (CommandLine::FindOption("ProfileRun"))
    {
        FFrameProfiler::Get().Enable();
        FFrameProfiler::Get().SetCaptureNativeStacks(CommandLine::FindOption("ProfileNativeStacks"));
        FFrameProfiler::Get().SetRetainAllFrames(true);

        if (IGPUProfiler* Profiler = GetGPUProfiler())
        {
            Profiler->Enable();
            Profiler->SetRetainAllFrames(true);
        }

        return;
    }

    const bool bActive = bIsProfilingRequested && IsVisible();
    if (bActive)
    {
        FFrameProfiler::Get().Enable();
    }
    else
    {
        FFrameProfiler::Get().Disable();
    }

    if (IGPUProfiler* Profiler = GetGPUProfiler())
    {
        if (bActive)
        {
            Profiler->Enable();
        }
        else
        {
            Profiler->Disable();
        }
    }

    FFrameProfiler::Get().SetCaptureNativeStacks(bCaptureNativeStacks);
}

void FEditorProfilerPanel::Tick(float DeltaTime)
{
    if (!IsVisible())
    {
        return;
    }

    ApplyProfilerState();

    TimeSinceDetailRefresh    += DeltaTime;
    TimeSinceAggregateRefresh += DeltaTime;

    RefreshHeader();
    if (bCollectOptimizationTargets && TimeSinceAggregateRefresh >= LIVE_REFRESH_INTERVAL)
    {
        TimeSinceAggregateRefresh = 0.0f;
        RefreshOptimizationTargets();
    }

    if (!bIsFrozen)
    {
        RefreshHistograms();
        RefreshFrameSelection();
        RefreshPipelineStatistics();
        RefreshWorstFrames();
    }

    const bool bIsThrottled  = bFollowLatestFrame && (TimeSinceDetailRefresh < LIVE_REFRESH_INTERVAL);
    const bool bAllowDetails = bDetailsAreDirty && (bIsFrozen || !bIsThrottled);

    if (bAllowDetails)
    {
        TimeSinceDetailRefresh = 0.0f;
        if (bIsFrozen)
        {
            FillTimeline();
            RebuildHierarchy();
            RebuildSelectionDetails();
            bDetailsAreDirty = false;
        }
        else
        {
            RefreshDetails();
        }
    }
}

void FEditorProfilerPanel::RefreshHeader()
{
    if (!SummaryText)
    {
        return;
    }

    const FFrameProfiler& Cpu = FFrameProfiler::Get();

    const float CpuMs = ShownFrameNumber >= 0 || Mode == EProfilerMode::Boot
        ? ShownFrame.CpuMilliseconds
        : Cpu.GetLatestCpuMilliseconds();

    const float GpuMs = bHasShownGpuFrame ? ShownGpuMilliseconds : 0.0f;

    const CHAR* Bound = GpuMs <= 0.0f ? "no GPU timings" : (CpuMs >= GpuMs ? "CPU-bound" : "GPU-bound");
    const CHAR* Which = Mode == EProfilerMode::Boot ? "boot session" : (bIsFrozen ? "frozen" : (bFollowLatestFrame ? "live" : "pinned"));

    SummaryText->SetText(String::Printf("%d FPS   CPU %.2f ms   GPU %.2f ms   %s   boot %.0f ms   frame %d (%s)%s%s",
        Cpu.GetFramesPerSecond(), CpuMs, GpuMs, Bound, FBootProfiler::Get().GetSession().CpuMilliseconds,
        ShownFrameNumber, Which, GpuStatusText.IsEmpty() ? "" : "   ", *GpuStatusText));
}

void FEditorProfilerPanel::RefreshHistograms()
{
    const FFrameProfiler& Cpu = FFrameProfiler::Get();

    const int32 CpuCount = Cpu.GetStoredFrameCount();
    for (int32 Index = 0; Index < CpuCount; ++Index)
    {
        const FProfilerFrame* Frame = Cpu.GetStoredFrame(Index);
        if (!Frame || Frame->FrameIndex <= LastIngestedCpuFrameIndex)
        {
            continue;
        }

        CpuHistogram->AddSample(Frame->CpuMilliseconds);
        LastIngestedCpuFrameIndex = Frame->FrameIndex;

        if (bFollowLatestFrame)
        {
            bDetailsAreDirty = true;
        }
    }

    if (Mode == EProfilerMode::Boot)
    {
        return;
    }

    if (IGPUProfiler* Gpu = GetGPUProfiler())
    {
        const int32 GpuCount = Gpu->GetStoredFrameCount();

        FProfilerGpuFrame GpuFrame;
        for (int32 Index = 0; Index < GpuCount; ++Index)
        {
            if (!Gpu->GetStoredFrame(Index, GpuFrame))
            {
                continue;
            }

            if (bHasIngestedGpuFrame && GpuFrame.CpuFrameIndex <= LastIngestedGpuCpuFrameIndex)
            {
                continue;
            }

            GpuHistogram->AddSample(GpuFrame.GpuMilliseconds);

            LastIngestedGpuCpuFrameIndex = GpuFrame.CpuFrameIndex;
            bHasIngestedGpuFrame         = true;
        }
    }
}

void FEditorProfilerPanel::RefreshFrameSelection()
{
    if (Mode != EProfilerMode::Frames)
    {
        return;
    }

    const FFrameProfiler& Cpu        = FFrameProfiler::Get();
    const int32           NumStored  = Cpu.GetStoredFrameCount();
    const int32           NumSamples = CpuHistogram->GetNumSamples();

    if (NumStored <= 0 || NumSamples <= 0)
    {
        return;
    }

    const int32 Clicked = CpuHistogram->GetSelectedSample();

    bool bCpuSelectionChanged = false;
    if (Clicked != FHistogram::InvalidSampleIndex && Clicked != AppliedHistogramSample)
    {
        bCpuSelectionChanged   = true;
        AppliedHistogramSample = Clicked;

        const int32 StepsBack = Math::Clamp(NumSamples - 1 - Clicked, 0, NumStored - 1);
        if (const FProfilerFrame* Frame = Cpu.GetStoredFrame(NumStored - 1 - StepsBack))
        {
            PinnedFrameNumber = Frame->FrameIndex;
            SetFollowLatestFrame(false);
        }

    }

    if (!bCpuSelectionChanged)
    {
        if (IGPUProfiler* Gpu = GetGPUProfiler())
        {
            const int32 GpuClicked = GpuHistogram->GetSelectedSample();
            const int32 GpuCount   = Gpu->GetStoredFrameCount();
            const int32 GpuSamples = GpuHistogram->GetNumSamples();

            if (GpuClicked != FHistogram::InvalidSampleIndex && GpuClicked != AppliedGpuHistogramSample && GpuCount > 0 && GpuSamples > 0)
            {
                AppliedGpuHistogramSample = GpuClicked;
                const int32 StepsBack = Math::Clamp(GpuSamples - 1 - GpuClicked, 0, GpuCount - 1);
                FProfilerGpuFrame GpuFrame;
                if (Gpu->GetStoredFrame(GpuCount - 1 - StepsBack, GpuFrame) && GpuFrame.CpuFrameIndex >= 0)
                {
                    PinnedFrameNumber = GpuFrame.CpuFrameIndex;
                    SetFollowLatestFrame(false);
                }
            }
        }
    }

    if (bFollowLatestFrame)
    {
        const int32 ShownIndex = FindStoredIndexForFrame(ShownFrameNumber);
        AppliedHistogramSample = ShownIndex >= 0
            ? Math::Max(NumSamples - 1 - (NumStored - 1 - ShownIndex), 0)
            : NumSamples - 1;

        AppliedGpuHistogramSample = GpuHistogram->GetNumSamples() - 1;
    }
    else
    {
        const int32 StoredIndex = FindStoredIndexForFrame(PinnedFrameNumber);
        if (StoredIndex < 0)
        {
            return;
        }

        AppliedHistogramSample = Math::Max(NumSamples - 1 - (NumStored - 1 - StoredIndex), 0);
        if (IGPUProfiler* Gpu = GetGPUProfiler())
        {
            const int32 GpuCount       = Gpu->GetStoredFrameCount();
            int32       GpuStoredIndex = -1;

            FProfilerGpuFrame GpuFrame;
            for (int32 Index = 0; Index < GpuCount; ++Index)
            {
                if (Gpu->GetStoredFrame(Index, GpuFrame) && GpuFrame.CpuFrameIndex == PinnedFrameNumber)
                {
                    GpuStoredIndex = Index;
                    break;
                }
            }

            if (GpuStoredIndex >= 0)
            {
                AppliedGpuHistogramSample = Math::Max(GpuHistogram->GetNumSamples() - 1 - (GpuCount - 1 - GpuStoredIndex), 0);
            }
        }
    }

    CpuHistogram->SetSelectedSample(AppliedHistogramSample);
    if (AppliedGpuHistogramSample >= 0)
    {
        GpuHistogram->SetSelectedSample(AppliedGpuHistogramSample);
    }
}

int32 FEditorProfilerPanel::FindStoredIndexForFrame(int32 FrameNumber) const
{
    if (FrameNumber < 0)
    {
        return -1;
    }

    const FFrameProfiler& Cpu = FFrameProfiler::Get();
    for (int32 Index = Cpu.GetStoredFrameCount() - 1; Index >= 0; --Index)
    {
        const FProfilerFrame* Frame = Cpu.GetStoredFrame(Index);
        if (Frame && Frame->FrameIndex == FrameNumber)
        {
            return Index;
        }
    }

    return -1;
}

const FProfilerFrame* FEditorProfilerPanel::ResolveSelectedFrame() const
{
    const FFrameProfiler& Cpu = FFrameProfiler::Get();

    const int32 NumStored = Cpu.GetStoredFrameCount();
    if (NumStored <= 0)
    {
        return nullptr;
    }

    if (!bFollowLatestFrame)
    {
        const int32 StoredIndex = FindStoredIndexForFrame(PinnedFrameNumber);
        if (StoredIndex >= 0)
        {
            return Cpu.GetStoredFrame(StoredIndex);
        }

        return Cpu.GetStoredFrame(NumStored - 1);
    }

    if (IGPUProfiler* Gpu = GetGPUProfiler())
    {
        FProfilerGpuFrame Latest;
        if (Gpu->GetLatestFrame(Latest) && Latest.CpuFrameIndex >= 0)
        {
            const int32 StoredIndex = FindStoredIndexForFrame(Latest.CpuFrameIndex);
            if (StoredIndex >= 0)
            {
                return Cpu.GetStoredFrame(StoredIndex);
            }
        }
    }

    return Cpu.GetStoredFrame(NumStored - 1);
}

void FEditorProfilerPanel::RefreshDetails()
{
    bDetailsAreDirty = false;

    GpuIntervals.Clear();
    GpuStatusText.Clear();

    ShownGpuMilliseconds = 0.0f;
    bHasShownGpuFrame    = false;

    if (Mode == EProfilerMode::Boot)
    {
        ShownFrame       = FBootProfiler::Get().GetSession();
        ShownFrameNumber = -1;
    }
    else
    {
        const FProfilerFrame* Frame = ResolveSelectedFrame();
        if (!Frame)
        {
            ShownFrame       = FProfilerFrame();
            ShownFrameNumber = -1;
        }
        else
        {
            ShownFrame       = *Frame;
            ShownFrameNumber = Frame->FrameIndex;
        }

        if (IGPUProfiler* Gpu = GetGPUProfiler())
        {
            FProfilerGpuFrame GpuFrame;
            if (ShownFrameNumber >= 0 && Gpu->FindFrameForCpuFrame(ShownFrameNumber, GpuFrame))
            {
                ShownGpuMilliseconds = GpuFrame.GpuMilliseconds;
                bHasShownGpuFrame    = true;
                GpuIntervals         = Move(GpuFrame.Intervals);
            }
            else
            {
                FProfilerGpuFrame Latest;
                if (!Gpu->GetLatestFrame(Latest))
                {
                    GpuStatusText = String("No GPU capture for this CPU frame");
                }
                else if (bFollowLatestFrame)
                {
                    if (Latest.CpuFrameIndex < 0)
                    {
                        GpuStatusText = String("not matched to a CPU frame");
                    }

                    ShownGpuMilliseconds = Latest.GpuMilliseconds;
                    bHasShownGpuFrame    = true;
                    GpuIntervals         = Move(Latest.Intervals);
                }
                else if (Latest.CpuFrameIndex >= 0 && ShownFrameNumber > Latest.CpuFrameIndex)
                {
                    GpuStatusText = String::Printf("GPU is %d frames behind", ShownFrameNumber - Latest.CpuFrameIndex);
                }
                else
                {
                    GpuStatusText = String("No GPU capture for this CPU frame");
                }
            }
        }
    }

    FillTimeline();
    RebuildHierarchy();
    RebuildSelectionDetails();
    RefreshPipelineStatistics();
}

void FEditorProfilerPanel::FillTimeline()
{
    TArray<FProfilerTimelineLane> Lanes;
    
    const uint64 Frequency     = FFrameProfiler::Get().GetFrequency();
    uint64       BaseTimeStamp = ShownFrame.StartTimeStamp;

    for (const FProfilerThreadFrame& ThreadFrame : ShownFrame.Threads)
    {
        for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
        {
            BaseTimeStamp = Math::Min(BaseTimeStamp, Interval.StartTimeStamp);
        }
    }

    for (int32 ThreadIndex = 0; ThreadIndex < ShownFrame.Threads.Size(); ++ThreadIndex)
    {
        const FProfilerThreadFrame& ThreadFrame = ShownFrame.Threads[ThreadIndex];

        FProfilerTimelineLane Lane;
        Lane.Label  = ResolveThreadName(ThreadFrame.ThreadHandle, ThreadIndex);
        Lane.bIsGpu = false;

        uint64 LaneNanoseconds = 0;
        for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
        {
            FProfilerTimelineBar Bar;
            Bar.Name             = Interval.Name;
            Bar.Depth            = Interval.Depth;
            Bar.ParentIndex      = Interval.ParentIndex;
            Bar.bInstant         = Interval.bInstant;
            Bar.bDimmed          = !SearchQuery.IsEmpty() && !NameContainsIgnoreCase(Interval.Name, SearchQuery);
            Bar.StartNanoseconds = ProfilerTicksToNanoseconds(Interval.StartTimeStamp - BaseTimeStamp, Frequency);
            Bar.EndNanoseconds   = Bar.StartNanoseconds + Interval.InclusiveNanoseconds;
            Lane.Bars.Add(Bar);

            if (Interval.Depth == 0)
            {
                LaneNanoseconds += Interval.InclusiveNanoseconds;
            }
        }

        Lane.SubLabel = String::Printf("%.2f ms", ToMilliseconds(LaneNanoseconds));
        Lanes.Add(Move(Lane));
    }

    if (Mode != EProfilerMode::Boot)
    {
        FProfilerTimelineLane GpuLane;
        GpuLane.Label  = "GPU";
        GpuLane.bIsGpu = true;

        uint64 GpuNanoseconds = 0;
        for (const FGPUProfilerInterval& Interval : GpuIntervals)
        {
            FProfilerTimelineBar Bar;
            Bar.Name             = Interval.Name;
            Bar.StartNanoseconds = Interval.StartNanoseconds;
            Bar.EndNanoseconds   = Interval.EndNanoseconds;
            Bar.Depth            = Interval.Depth;
            Bar.ParentIndex      = Interval.ParentIndex;
            Bar.bInstant         = Interval.bInstant;
            Bar.bDimmed          = !SearchQuery.IsEmpty() && !NameContainsIgnoreCase(Interval.Name, SearchQuery);
            GpuLane.Bars.Add(Bar);

            if (Interval.Depth == 0)
            {
                GpuNanoseconds += Interval.InclusiveNanoseconds;
            }
        }

        if (!GpuStatusText.IsEmpty())
        {
            GpuLane.SubLabel = GpuStatusText;
        }
        else
        {
            GpuLane.SubLabel = GpuIntervals.IsEmpty() ? String("no queries") : String::Printf("%.2f ms", ToMilliseconds(GpuNanoseconds));
        }

        Lanes.Add(Move(GpuLane));
    }

    Timeline->SetLanes(Move(Lanes));
    Timeline->SetSelectedBar(Selection.LaneIndex, Selection.IntervalIndex);
}

TSharedPtr<FTreeItem> FEditorProfilerPanel::BuildCallTree(
    int32                      LaneIndex,
    const String&              LaneLabel,
    const TArray<const CHAR*>& Names,
    const TArray<int32>&       Parents,
    const TArray<uint64>&      Inclusive,
    const TArray<uint64>&      Exclusive,
    bool                       bIsGpu)
{
    TSharedPtr<FTreeItem> LaneItem = FTreeItem::Create(LaneLabel);
    LaneItem->bIsExpanded = true;

    TArray<FCallTreeNode> Nodes;
    TArray<int32>         IntervalToNode;
    IntervalToNode.Resize(Names.Size());

    TArray<TSharedPtr<FTreeItem>> IntervalItems;
    IntervalItems.Resize(Names.Size());

    int32  LaneCalls       = 0;
    uint64 LaneNanoseconds = 0;

    for (int32 Index = 0; Index < Names.Size(); ++Index)
    {
        const int32 ParentInterval = Parents[Index];
        const int32 ParentNode     = (ParentInterval >= 0 && ParentInterval < Index) ? IntervalToNode[ParentInterval] : -1;

        int32 NodeIndex = -1;
        for (int32 Search = 0; Search < Nodes.Size(); ++Search)
        {
            if (Nodes[Search].ParentNode == ParentNode && AreScopeNamesEqual(Nodes[Search].Name, Names[Index]))
            {
                NodeIndex = Search;
                break;
            }
        }

        if (NodeIndex < 0)
        {
            FCallTreeNode Node;
            Node.Name       = Names[Index];
            Node.ParentNode = ParentNode;
            Node.Item       = FTreeItem::Create(Names[Index] ? String(Names[Index]) : String("<unnamed>"));

            NodeIndex = Nodes.Size();
            Nodes.Add(Node);

            const int32 RefIndex = HierarchyRefs.Size();

            FScopeRef Ref;
            Ref.LaneIndex     = LaneIndex;
            Ref.IntervalIndex = Index;
            Ref.bIsGpu        = bIsGpu;
            HierarchyRefs.Add(Ref);

            Nodes[NodeIndex].Item->UserData = reinterpret_cast<void*>(static_cast<UPTR_INT>(RefIndex + 1));

            if (ParentNode >= 0)
            {
                Nodes[ParentNode].Item->AddChild(Nodes[NodeIndex].Item);
            }
            else
            {
                LaneItem->AddChild(Nodes[NodeIndex].Item);
            }
        }

        FCallTreeNode& Node = Nodes[NodeIndex];
        Node.Calls++;
        Node.InclusiveNanoseconds += Inclusive[Index];
        Node.ExclusiveNanoseconds += Exclusive[Index];

        if (Inclusive[Index] >= Node.LongestInclusive)
        {
            Node.LongestInclusive = Inclusive[Index];
            Node.LongestInterval  = Index;

            const UPTR_INT RefIndex = reinterpret_cast<UPTR_INT>(Node.Item->UserData);
            if (RefIndex > 0)
            {
                HierarchyRefs[static_cast<int32>(RefIndex) - 1].IntervalIndex = Index;
            }
        }

        IntervalToNode[Index] = NodeIndex;
        IntervalItems[Index]  = Node.Item;

        if (ParentNode < 0)
        {
            LaneCalls++;
            LaneNanoseconds += Inclusive[Index];
        }
    }

    const double LaneTotal = static_cast<double>(LaneNanoseconds);
    for (FCallTreeNode& Node : Nodes)
    {
        const double SharePercent = LaneTotal > 0.0
            ? (static_cast<double>(Node.InclusiveNanoseconds) * 100.0) / LaneTotal
            : 0.0;

        Node.Item->TypeLabel   = FormatScopeColumns(Node.Calls, Node.InclusiveNanoseconds, Node.ExclusiveNanoseconds, SharePercent);
        Node.Item->bIsExpanded = Node.Item->GetDepth() < PROFILER_DEFAULT_EXPAND_DEPTH;
    }

    LaneItem->TypeLabel = FormatScopeColumns(LaneCalls, LaneNanoseconds, 0, LaneTotal > 0.0 ? 100.0 : 0.0);
    LaneIntervalItems.Add(Move(IntervalItems));
    return LaneItem;
}

void FEditorProfilerPanel::RebuildHierarchy()
{
    HierarchyRoots.Clear();
    HierarchyRefs.Clear();
    LaneIntervalItems.Clear();

    TArray<const CHAR*> Names;
    TArray<int32>       Parents;
    TArray<uint64>      Inclusive;
    TArray<uint64>      Exclusive;

    for (int32 ThreadIndex = 0; ThreadIndex < ShownFrame.Threads.Size(); ++ThreadIndex)
    {
        const FProfilerThreadFrame& ThreadFrame = ShownFrame.Threads[ThreadIndex];

        Names.Clear();
        Parents.Clear();
        Inclusive.Clear();
        Exclusive.Clear();

        for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
        {
            Names.Add(Interval.Name);
            Parents.Add(Interval.ParentIndex);
            Inclusive.Add(Interval.InclusiveNanoseconds);
            Exclusive.Add(Interval.ExclusiveNanoseconds);
        }

        HierarchyRoots.Add(BuildCallTree(ThreadIndex, ResolveThreadName(ThreadFrame.ThreadHandle, ThreadIndex),
            Names, Parents, Inclusive, Exclusive, false));
    }

    if (Mode != EProfilerMode::Boot)
    {
        Names.Clear();
        Parents.Clear();
        Inclusive.Clear();
        Exclusive.Clear();

        for (const FGPUProfilerInterval& Interval : GpuIntervals)
        {
            Names.Add(Interval.Name);
            Parents.Add(Interval.ParentIndex);
            Inclusive.Add(Interval.InclusiveNanoseconds);
            Exclusive.Add(Interval.ExclusiveNanoseconds);
        }

        TSharedPtr<FTreeItem> GpuItem = BuildCallTree(ShownFrame.Threads.Size(), String("GPU"),
            Names, Parents, Inclusive, Exclusive, true);

        if (GpuIntervals.IsEmpty())
        {
            GpuItem->TypeLabel = String("no queries resolved");
        }

        HierarchyRoots.Add(GpuItem);
    }

    HierarchyTree->SetRootItems(HierarchyRoots);
    ApplySearchToHierarchy();
    if (Selection.LaneIndex >= 0)
    {
        SelectTreeItemForScope(Selection);
    }
}

String FEditorProfilerPanel::FormatScopeColumns(int32 Calls, uint64 InclusiveNanoseconds, uint64 ExclusiveNanoseconds, double LanePercent)
{
    return String::Printf("%5d %10.3f %10.3f %8.1f%%", Calls, ToMilliseconds(InclusiveNanoseconds), ToMilliseconds(ExclusiveNanoseconds), LanePercent);
}

String FEditorProfilerPanel::FormatTargetColumns(double CallsPerFrame, double InclusiveMilliseconds, double ExclusiveMilliseconds, double BudgetPercent)
{
    return String::Printf("%6.2f %10.3f %10.3f %8.1f%%", CallsPerFrame, InclusiveMilliseconds, ExclusiveMilliseconds, BudgetPercent);
}

void FEditorProfilerPanel::OnTargetSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& InSelection)
{
    if (InSelection.IsEmpty() || !InSelection[0] || !InSelection[0]->UserData)
    {
        return;
    }

    const int32 Rank = static_cast<int32>(reinterpret_cast<uintptr_t>(InSelection[0]->UserData));
    if (Rank < 1 || Rank > TargetScopeNames.Size())
    {
        return;
    }

    const String& Name = TargetScopeNames[Rank - 1];
    if (SearchBox)
    {
        SearchBox->SetText(Name);
    }

    OnSearchTextChanged(Name);
}

void FEditorProfilerPanel::OnTreeExpansionChanged(const TSharedPtr<FTreeItem>& /*Item*/, bool /*bIsExpanded*/)
{
    PinCurrentFrame();
}

void FEditorProfilerPanel::OnTreeSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& InSelection)
{
    Selection = FScopeRef();

    if (!InSelection.IsEmpty() && InSelection[0])
    {
        const UPTR_INT RefIndex = reinterpret_cast<UPTR_INT>(InSelection[0]->UserData);
        if (RefIndex > 0 && static_cast<int32>(RefIndex) - 1 < HierarchyRefs.Size())
        {
            Selection = HierarchyRefs[static_cast<int32>(RefIndex) - 1];
        }
    }

    PinCurrentFrame();
    Timeline->SetSelectedBar(Selection.LaneIndex, Selection.IntervalIndex);
    RebuildSelectionDetails();
}

void FEditorProfilerPanel::OnTimelineBarSelected(int32 LaneIndex, int32 BarIndex)
{
    Selection = FScopeRef();
    Selection.LaneIndex     = LaneIndex;
    Selection.IntervalIndex = BarIndex;
    Selection.bIsGpu        = LaneIndex >= 0 && LaneIndex >= ShownFrame.Threads.Size();

    if (LaneIndex >= 0)
    {
        PinCurrentFrame();
    }
    else if (HierarchyTree)
    {
        HierarchyTree->ClearSelection();
    }

    SelectTreeItemForScope(Selection);
    RebuildSelectionDetails();
}

TSharedPtr<FVisualElement> FEditorProfilerPanel::BuildTimelineBarContextMenu(int32 LaneIndex, int32 BarIndex)
{
    if (LaneIndex != ShownFrame.Threads.Size() || !GpuIntervals.IsValidIndex(BarIndex)
        || !GpuIntervals[BarIndex].Name)
    {
        return nullptr;
    }

    return BuildGpuPassContextMenu(String(GpuIntervals[BarIndex].Name));
}

TSharedPtr<FVisualElement> FEditorProfilerPanel::BuildScopeContextMenu(const TSharedPtr<FTreeItem>& Item)
{
    if (!Item || !Item->UserData)
    {
        return nullptr;
    }

    const UPTR_INT RefIndex = reinterpret_cast<UPTR_INT>(Item->UserData);
    if (RefIndex == 0 || static_cast<int32>(RefIndex) - 1 >= HierarchyRefs.Size())
    {
        return nullptr;
    }

    const FScopeRef& Scope = HierarchyRefs[static_cast<int32>(RefIndex) - 1];
    if (!Scope.bIsGpu || !GpuIntervals.IsValidIndex(Scope.IntervalIndex)
        || !GpuIntervals[Scope.IntervalIndex].Name)
    {
        return nullptr;
    }

    return BuildGpuPassContextMenu(String(GpuIntervals[Scope.IntervalIndex].Name));
}

TSharedPtr<FVisualElement> FEditorProfilerPanel::BuildGpuPassContextMenu(const String& PassName)
{
    if (PassName.IsEmpty())
    {
        return nullptr;
    }

    TSharedPtr<FMenu> Menu = FMenu::Create();

    FMenuItem::FDesc ItemDesc;
    ItemDesc.Label       = "Show in Render Graph";
    ItemDesc.Font        = FEditorStyle::GetFonts().Body;
    ItemDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this, PassName]()
    {
        ShowGpuPassInRenderGraph(PassName);
    });

    Menu->AddItem(FMenuItem::Create(ItemDesc));
    return Menu;
}

void FEditorProfilerPanel::ShowGpuPassInRenderGraph(const String& PassName)
{
    if (!EditorEngine || !EditorEngine->GetEditorShell() || !EditorEngine->GetEditorShell()->GetRegistry())
    {
        return;
    }

    TSharedPtr<FEditorPanelRegistry> Registry = EditorEngine->GetEditorShell()->GetRegistry();
    if (TSharedPtr<FEditorPanel> Panel = Registry->FindPanel("RenderGraph"))
    {
        static_cast<FEditorRenderGraphPanel*>(Panel.Get())->ShowPassByName(PassName);
    }
}

void FEditorProfilerPanel::SelectTreeItemForScope(const FScopeRef& Scope)
{
    if (!HierarchyTree)
    {
        return;
    }

    if (Scope.LaneIndex < 0 || Scope.IntervalIndex < 0
        || Scope.LaneIndex >= LaneIntervalItems.Size()
        || Scope.IntervalIndex >= LaneIntervalItems[Scope.LaneIndex].Size())
    {
        return;
    }

    TSharedPtr<FTreeItem> Item = LaneIntervalItems[Scope.LaneIndex][Scope.IntervalIndex];
    if (!Item)
    {
        return;
    }

    for (TSharedPtr<FTreeItem> Walk = Item->Parent.ToSharedPtr(); Walk; Walk = Walk->Parent.ToSharedPtr())
    {
        HierarchyTree->SetItemExpanded(Walk, true);
    }

    TArray<TSharedPtr<FTreeItem>> Selected;
    Selected.Add(Item);
    HierarchyTree->SetSelection(Selected);
    HierarchyTree->ScrollToItem(Item);
}

void FEditorProfilerPanel::ApplySearchToHierarchy()
{
    if (!HierarchyTree)
    {
        return;
    }

    HierarchyTree->SetFilterText(SearchQuery);

    if (SearchQuery.IsEmpty())
    {
        auto RestoreDefaultExpansion = [&](auto& Self, const TSharedPtr<FTreeItem>& Item) -> void
        {
            if (!Item)
            {
                return;
            }

            HierarchyTree->SetItemExpanded(Item, Item->GetDepth() < PROFILER_DEFAULT_EXPAND_DEPTH);
            for (const TSharedPtr<FTreeItem>& Child : Item->Children)
            {
                Self(Self, Child);
            }
        };

        for (const TSharedPtr<FTreeItem>& Root : HierarchyRoots)
        {
            RestoreDefaultExpansion(RestoreDefaultExpansion, Root);
        }

        return;
    }

    TSharedPtr<FTreeItem> FirstMatch;
    auto FindFirstMatch = [&](auto& Self, const TSharedPtr<FTreeItem>& Item) -> void
    {
        if (!Item || FirstMatch)
        {
            return;
        }

        if (NameContainsIgnoreCase(*Item->Label, SearchQuery))
        {
            FirstMatch = Item;
            return;
        }

        for (const TSharedPtr<FTreeItem>& Child : Item->Children)
        {
            Self(Self, Child);
        }
    };

    for (const TSharedPtr<FTreeItem>& Root : HierarchyRoots)
    {
        FindFirstMatch(FindFirstMatch, Root);
    }

    if (FirstMatch)
    {
        HierarchyTree->ScrollToItem(FirstMatch);
    }
}

void FEditorProfilerPanel::OnSearchTextChanged(const String& Text)
{
    SearchQuery      = Text;
    bDetailsAreDirty = true;

    if (bIsFrozen)
    {
        FillTimeline();
        ApplySearchToHierarchy();
        bDetailsAreDirty = false;
    }
}

void FEditorProfilerPanel::RebuildSelectionDetails()
{
    RebuildCallers();
    RebuildNativeStack();
    RefreshPipelineStatistics();
}

void FEditorProfilerPanel::RebuildCallers()
{
    CallersTable->ClearRows();
    CallersTable->AddHeaderRow("Marker callers");

    if (Selection.LaneIndex < 0 || Selection.IntervalIndex < 0)
    {
        CallersTable->AddRow("Select a scope in the timeline or the tree", CreateValueText(""));
        return;
    }

    TArray<const CHAR*> Names;
    TArray<int32>       Parents;
    TArray<uint64>      Inclusive;

    if (Selection.bIsGpu)
    {
        for (const FGPUProfilerInterval& Interval : GpuIntervals)
        {
            Names.Add(Interval.Name);
            Parents.Add(Interval.ParentIndex);
            Inclusive.Add(Interval.InclusiveNanoseconds);
        }
    }
    else if (Selection.LaneIndex < ShownFrame.Threads.Size())
    {
        for (const FProfilerInterval& Interval : ShownFrame.Threads[Selection.LaneIndex].Intervals)
        {
            Names.Add(Interval.Name);
            Parents.Add(Interval.ParentIndex);
            Inclusive.Add(Interval.InclusiveNanoseconds);
        }
    }

    if (Selection.IntervalIndex >= Names.Size())
    {
        CallersTable->AddRow("The selected scope is no longer in the shown frame", CreateValueText(""));
        return;
    }

    TArray<int32> Callers;
    for (int32 Walk = Parents[Selection.IntervalIndex]; Walk >= 0 && Walk < Parents.Size(); Walk = Parents[Walk])
    {
        Callers.Add(Walk);
    }

    for (int32 Index = Callers.Size() - 1; Index >= 0; --Index)
    {
        const int32 Caller = Callers[Index];
        CallersTable->AddRow(Names[Caller] ? Names[Caller] : "<unnamed>",
            CreateValueText(String::Printf("%.3f ms", ToMilliseconds(Inclusive[Caller]))));
    }

    FPropertyRow& Row = CallersTable->AddRow(Names[Selection.IntervalIndex] ? Names[Selection.IntervalIndex] : "<unnamed>",
        CreateValueText(String::Printf("%.3f ms   selected", ToMilliseconds(Inclusive[Selection.IntervalIndex]))));
    Row.IndentLevel = Callers.Size();
}

void FEditorProfilerPanel::RebuildNativeStack()
{
    NativeStackTable->ClearRows();
    NativeStackTable->AddHeaderRow("Native stack");

    if (Selection.bIsGpu)
    {
        NativeStackTable->AddRow("GPU scopes are recorded on the queue, so they carry no CPU stack", CreateValueText(""));
        return;
    }

    if (Selection.LaneIndex < 0 || Selection.LaneIndex >= ShownFrame.Threads.Size() || Selection.IntervalIndex < 0)
    {
        NativeStackTable->AddRow("Select a scope in the timeline or the tree", CreateValueText(""));
        return;
    }

    const TArray<FProfilerInterval>& Intervals = ShownFrame.Threads[Selection.LaneIndex].Intervals;
    if (Selection.IntervalIndex >= Intervals.Size())
    {
        NativeStackTable->AddRow("The selected scope is no longer in the shown frame", CreateValueText(""));
        return;
    }

    const FProfilerInterval& Interval = Intervals[Selection.IntervalIndex];
    if (Interval.StackDepth < 1)
    {
        NativeStackTable->AddRow(bCaptureNativeStacks
            ? "No stack was captured for this scope"
            : "Turn on Native stacks, then wait for the scope to be recorded again", CreateValueText(""));
        return;
    }

    FPlatformStackTrace::InitializeSymbols();
    for (int32 Frame = 0; Frame < Interval.StackDepth && Frame < NUM_PROFILER_STACK_FRAMES; ++Frame)
    {
        FStackTraceEntry Entry;
        FPlatformStackTrace::GetStackTraceEntryFromAddress(Interval.StackFrames[Frame], Entry);
        NativeStackTable->AddRow(Entry.FunctionName, CreateValueText(String::Printf("%s:%u", Entry.Filename, Entry.Line)));
    }

    FPlatformStackTrace::ReleaseSymbols();
}

void FEditorProfilerPanel::BuildPipelineStatisticsRows()
{
    PipelineStatisticsTable->AddHeaderRow("Pipeline statistics");
    PipelineStatisticsContext = CreateValueText("Not measured");
    PipelineStatisticsTable->AddRow("Context", PipelineStatisticsContext);

    for (const FPipelineStatisticsCounter& Counter : GPipelineStatisticsCounters)
    {
        TSharedPtr<FTextBlock> Value = CreateValueText("0");
        PipelineStatisticsTable->AddRow(Counter.Label, Value);
        PipelineStatisticsValues.Emplace(Value);
    }
}

void FEditorProfilerPanel::BindPipelineStatistics(const FRHIPipelineStatistics* Statistics)
{
    for (int32 Index = 0; Index < PipelineStatisticsValues.Size(); ++Index)
    {
        if (!Statistics)
        {
            PipelineStatisticsValues[Index]->SetText(String("-"));
            continue;
        }

        const uint64 Counter = Statistics->*GPipelineStatisticsCounters[Index].Member;
        PipelineStatisticsValues[Index]->SetText(String::Printf("%llu", static_cast<unsigned long long>(Counter)));
    }
}

void FEditorProfilerPanel::RefreshPipelineStatistics()
{
    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
    {
        PipelineStatisticsContext->SetText(String("GPU profiler unavailable"));
        BindPipelineStatistics(nullptr);
        return;
    }

    if (!Profiler->IsPipelineStatisticsEnabled())
    {
        PipelineStatisticsContext->SetText(
            String("Not measured - enable Pipeline Statistics and wait for a new GPU frame"));
        BindPipelineStatistics(nullptr);
        return;
    }

    if (!bHasShownGpuFrame)
    {
        PipelineStatisticsContext->SetText(ShownFrameNumber >= 0
            ? String::Printf("Frame %d - no correlated GPU capture", ShownFrameNumber)
            : String("No GPU frame selected"));

        BindPipelineStatistics(nullptr);
        return;
    }

    if (Selection.bIsGpu && Selection.IntervalIndex >= 0 && Selection.IntervalIndex < GpuIntervals.Size())
    {
        const FGPUProfilerInterval& Interval = GpuIntervals[Selection.IntervalIndex];
    
        const CHAR* Name = Interval.Name ? Interval.Name : "<unnamed>";
        if (Interval.bHasPipelineStats)
        {
            PipelineStatisticsContext->SetText(
                String::Printf("Frame %d / pass %s", ShownFrameNumber, Name));
            BindPipelineStatistics(&Interval.PipelineStats);
        }
        else
        {
            PipelineStatisticsContext->SetText(
                String::Printf("Frame %d / pass %s - not measured", ShownFrameNumber, Name));
            BindPipelineStatistics(nullptr);
        }

        return;
    }

    bool bHasStatistics = false;
    for (const FGPUProfilerInterval& Interval : GpuIntervals)
    {
        bHasStatistics = bHasStatistics || Interval.bHasPipelineStats;
    }

    if (!bHasStatistics)
    {
        PipelineStatisticsContext->SetText(
            String::Printf("Frame %d - no pass statistics were measured", ShownFrameNumber));
        BindPipelineStatistics(nullptr);
        return;
    }

    const FRHIPipelineStatistics Total = SumPipelineStatistics(GpuIntervals);
    PipelineStatisticsContext->SetText(
        String::Printf("Frame %d / measured top-level GPU passes", ShownFrameNumber));
    BindPipelineStatistics(&Total);
}

void FEditorProfilerPanel::RefreshOptimizationTargets()
{
    if (!TargetsTree)
    {
        return;
    }

    TargetScopeNames.Clear();

    const bool bIsGpu = TargetDomain == ETargetDomain::Gpu;

    TArray<TSharedPtr<FTreeItem>> Roots;
    if (!bCollectOptimizationTargets)
    {
        Roots.Add(FTreeItem::Create("Turn Targets on in the toolbar to rank the stored frames"));
        TargetsTree->SetRootItems(Roots);
        return;
    }

    const TArray<FProfilerOptimizationTarget> Targets = bIsGpu
        ? CollectGpuOptimizationTargets()
        : FProfilerReport::CollectOptimizationTargets();

    if (Targets.IsEmpty())
    {
        Roots.Add(FTreeItem::Create(bIsGpu ? "No resolved GPU passes yet" : "No captured scopes yet"));
        TargetsTree->SetRootItems(Roots);
        return;
    }

    Roots.Reserve(Targets.Size());
    TargetScopeNames.Reserve(Targets.Size());

    for (int32 Index = 0; Index < Targets.Size(); ++Index)
    {
        const FProfilerOptimizationTarget& Target = Targets[Index];
        const int32                        Rank   = Index + 1;

        TSharedPtr<FTreeItem> Item = FTreeItem::Create(String::Printf("%2d. %s", Rank, *Target.Name));
        Item->TypeLabel = FormatTargetColumns(Target.CallsPerFrame, Target.InclusiveMillisecondsPerFrame,
            Target.SelfMillisecondsPerFrame, Target.BudgetPercent);
        Item->UserData  = reinterpret_cast<void*>(static_cast<uintptr_t>(Rank));
        Roots.Add(Item);
        TargetScopeNames.Add(Target.Name);
    }

    TargetsTree->SetRootItems(Roots);
}

TArray<FProfilerOptimizationTarget> FEditorProfilerPanel::CollectGpuOptimizationTargets() const
{
    struct FHotPass
    {
        String Name;
        uint64 Inclusive = 0;
        uint64 Exclusive = 0;
        int32  Calls     = 0;
    };

    TArray<FProfilerOptimizationTarget> Targets;

    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
    {
        return Targets;
    }

    TMap<String, FHotPass> HotPasses;

    double CapturedGpuMilliseconds = 0.0;
    int32  CapturedFrames          = 0;

    const int32 StoredFrames = Profiler->GetStoredFrameCount();
    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        FProfilerGpuFrame Frame;
        if (!Profiler->GetStoredFrame(FrameIndex, Frame))
        {
            continue;
        }

        CapturedGpuMilliseconds += static_cast<double>(Frame.GpuMilliseconds);
        CapturedFrames++;

        for (const FGPUProfilerInterval& Interval : Frame.Intervals)
        {
            const String Name = Interval.Name ? String(Interval.Name) : String("<unnamed>");

            FHotPass* Entry = HotPasses.Find(Name);
            if (!Entry)
            {
                FHotPass& Added = HotPasses.Add(Name);
                Added.Name = Name;
                Entry = &Added;
            }

            Entry->Inclusive += Interval.InclusiveNanoseconds;
            Entry->Exclusive += Interval.ExclusiveNanoseconds;
            Entry->Calls++;
        }
    }

    if (CapturedFrames <= 0)
    {
        return Targets;
    }

    TArray<FHotPass> Sorted;
    Sorted.Reserve(HotPasses.Size());

    for (auto Pair : HotPasses)
    {
        Sorted.Add(Pair.Second);
    }

    Sorted.SortWithPredicate([](const FHotPass& Left, const FHotPass& Right)
    {
        return Left.Exclusive > Right.Exclusive;
    });

    const double FrameDivisor  = static_cast<double>(CapturedFrames);
    const double CapturedGpuNs = CapturedGpuMilliseconds * 1000000.0;
    const int32  Limit         = Math::Min(Sorted.Size(), PROFILER_MAX_OPTIMIZATION_TARGETS);

    Targets.Reserve(Limit);
    for (int32 Index = 0; Index < Limit; ++Index)
    {
        const FHotPass& Pass = Sorted[Index];

        FProfilerOptimizationTarget Target;
        Target.Name                          = Pass.Name;
        Target.SelfMillisecondsPerFrame      = Time::ToMilliseconds(static_cast<double>(Pass.Exclusive)) / FrameDivisor;
        Target.InclusiveMillisecondsPerFrame = Time::ToMilliseconds(static_cast<double>(Pass.Inclusive)) / FrameDivisor;
        Target.CallsPerFrame                 = static_cast<double>(Pass.Calls) / FrameDivisor;
        Target.BudgetPercent                 = CapturedGpuNs > 0.0
            ? (static_cast<double>(Pass.Exclusive) * 100.0) / CapturedGpuNs
            : 0.0;

        Targets.Add(Target);
    }

    return Targets;
}

void FEditorProfilerPanel::RefreshWorstFrames()
{
    if (!WorstFramesRow)
    {
        return;
    }

    WorstFramesRow->ClearSlots();

    const TArray<FProfilerWorstFrame> Worst = FProfilerReport::CollectWorstFrames();
    if (Worst.Size() < 2)
    {
        return;
    }

    for (const FProfilerWorstFrame& Hitch : Worst)
    {
        const int32 FrameIndex = Hitch.FrameIndex;

        FButton::FDesc Desc;
        Desc.Font      = FEditorStyle::GetFonts().Body;
        Desc.bIsGhost  = true;
        Desc.Text      = String::Printf("%d  %.1fms", Hitch.FrameIndex, Hitch.CpuMilliseconds);
        Desc.OnClicked = FOnClicked::CreateLambda([this, FrameIndex]()
        {
            if (FindStoredIndexForFrame(FrameIndex) >= 0)
            {
                PinnedFrameNumber = FrameIndex;
                SetFollowLatestFrame(false);
                bDetailsAreDirty = true;
            }
        });

        WorstFramesRow->AddSlot(FButton::Create(Desc)).SetPadding(FMargin(0, 0, 6, 0));
    }
}

void FEditorProfilerPanel::SelectGpuScopeByName(const String& Name)
{
    if (Name.IsEmpty())
    {
        return;
    }

    if (Mode == EProfilerMode::Boot)
    {
        SetMode(EProfilerMode::Frames);
    }

    IGPUProfiler* Gpu = GetGPUProfiler();
    if (!Gpu)
    {
        return;
    }

    int32 MatchingCpuFrame = -1;
    FProfilerGpuFrame Candidate;
    for (int32 FrameIndex = Gpu->GetStoredFrameCount() - 1; FrameIndex >= 0; --FrameIndex)
    {
        if (!Gpu->GetStoredFrame(FrameIndex, Candidate) || Candidate.CpuFrameIndex < 0
            || !FFrameProfiler::Get().FindFrame(Candidate.CpuFrameIndex))
        {
            continue;
        }

        bool bContainsScope = false;
        for (const FGPUProfilerInterval& Interval : Candidate.Intervals)
        {
            if (AreScopeNamesEqual(Interval.Name, *Name))
            {
                bContainsScope = true;
                break;
            }
        }

        if (bContainsScope)
        {
            MatchingCpuFrame = Candidate.CpuFrameIndex;
            break;
        }
    }

    if (MatchingCpuFrame < 0)
    {
        return;
    }

    PinnedFrameNumber = MatchingCpuFrame;
    SetFollowLatestFrame(false);
    RefreshDetails();

    const int32 GpuLane = ShownFrame.Threads.Size();

    int32 Match    = -1;
    int32 Fallback = -1;

    for (int32 Index = 0; Index < GpuIntervals.Size(); ++Index)
    {
        if (!AreScopeNamesEqual(GpuIntervals[Index].Name, *Name))
        {
            continue;
        }

        Fallback = Index;
        if (GpuIntervals[Index].Depth == 0)
        {
            Match = Index;
            break;
        }
    }

    const int32 Chosen = Match >= 0 ? Match : Fallback;
    if (Chosen < 0)
    {
        return;
    }

    Selection.LaneIndex     = GpuLane;
    Selection.IntervalIndex = Chosen;
    Selection.bIsGpu        = true;

    Timeline->SetSelectedBar(Selection.LaneIndex, Selection.IntervalIndex);
    SelectTreeItemForScope(Selection);
    RebuildSelectionDetails();
}

void FEditorProfilerPanel::WriteCapture()
{
    String Text = FProfilerReport::BuildCpuAndBootText();
    Text.Append("\n== GPU ==\n");

    if (IGPUProfiler* Gpu = GetGPUProfiler())
    {
        FProfilerGpuFrame Latest;

        float  MinMs = TNumericLimits<float>::Max();
        float  MaxMs = TNumericLimits<float>::Lowest();
        double Sum   = 0.0;
        int32  Count = 0;

        FProfilerGpuFrame Stored;
        for (int32 Index = 0; Index < Gpu->GetStoredFrameCount(); ++Index)
        {
            if (!Gpu->GetStoredFrame(Index, Stored))
            {
                continue;
            }

            MinMs = Math::Min(MinMs, Stored.GpuMilliseconds);
            MaxMs = Math::Max(MaxMs, Stored.GpuMilliseconds);
            Sum  += static_cast<double>(Stored.GpuMilliseconds);

            Count++;

            Latest = Stored;
        }

        Text.Append(String::Printf("GPU frame avg=%.3f ms  min=%.3f  max=%.3f\n",
            Count > 0 ? static_cast<float>(Sum / static_cast<double>(Count)) : 0.0f,
            MinMs == TNumericLimits<float>::Max() ? 0.0f : MinMs,
            MaxMs == TNumericLimits<float>::Lowest() ? 0.0f : MaxMs));

        Text.Append("\nGPU pass tree (latest collected frame):\n");
        for (const FGPUProfilerInterval& Interval : Latest.Intervals)
        {
            for (int32 Depth = 0; Depth < Interval.Depth; ++Depth)
            {
                Text.Append("  ");
            }

            Text.Append(String::Printf("%s  inc=%.3f ms  exc=%.3f ms\n",
                Interval.Name ? Interval.Name : "<unnamed>",
                ToMilliseconds(Interval.InclusiveNanoseconds),
                ToMilliseconds(Interval.ExclusiveNanoseconds)));
        }
    }

    const uint64 Stamp    = FPlatformTime::QueryPerformanceCounter();
    const String TxtPath  = File::CombinePath(Paths::GetProjectDir(), String::Printf("Profiling/ProfileCapture_%llu.txt", Stamp));
    const String JsonPath = File::CombinePath(Paths::GetProjectDir(), String::Printf("Profiling/ProfileCapture_%llu.json", Stamp));

    FProfilerReport::WriteFile(TxtPath, Text);
    FProfilerReport::WriteFile(JsonPath, FProfilerReport::MergeChromeJson(FProfilerReport::BuildChromeCpuJson(), BuildChromeGpuEvents()));
}

String FEditorProfilerPanel::ResolveThreadName(void* ThreadHandle, int32 ThreadIndex)
{
    if (FThreadManager::Get().IsMainThread(ThreadHandle))
    {
        return String("GameThread");
    }

    if (IPlatformThread* Thread = FThreadManager::Get().GetThreadFromHandle(ThreadHandle))
    {
        return Thread->GetName();
    }

    return String::Printf("Thread %d", ThreadIndex);
}

TSharedPtr<FTextBlock> FEditorProfilerPanel::CreateValueText(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Monospace;
    return FTextBlock::Create(Desc);
}
