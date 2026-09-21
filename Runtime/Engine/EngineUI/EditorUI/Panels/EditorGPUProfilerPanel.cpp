#include "Engine/EngineUI/EditorUI/Panels/EditorGPUProfilerPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/Histogram.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/ToolBar.h"
#include "Core/Misc/ProfilerTypes.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Time/Time.h"
#include "RendererCore/Interfaces/IRendererModule.h"

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

FEditorGPUProfilerPanel::FEditorGPUProfilerPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "GPUProfiler", "GPU Profiler")
    , ToolBar(nullptr)
    , FrameTimeHistogram(nullptr)
    , PassTable(nullptr)
    , PipelineStatisticsTable(nullptr)
    , PassValues()
    , PipelineStatisticsValues()
    , LastIngestedCpuFrameIndex(-1)
    , bIsProfilingRequested(false)
{
}

FEditorGPUProfilerPanel::~FEditorGPUProfilerPanel()
{
}

bool FEditorGPUProfilerPanel::Initialize()
{
    ToolBar = BuildToolBar();
    if (!ToolBar)
    {
        return false;
    }

    FHistogram::FDesc HistogramDesc;
    HistogramDesc.Capacity        = NUM_LIVE_PROFILER_FRAMES;
    HistogramDesc.Font            = FEditorStyle::GetFonts().Body;
    HistogramDesc.Label           = "GPU Frame Time (ms)";
    HistogramDesc.PreferredHeight = 80;

    FrameTimeHistogram = FHistogram::Create(HistogramDesc);
    if (!FrameTimeHistogram)
    {
        return false;
    }

    PassTable               = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
    PipelineStatisticsTable = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());

    if (!PassTable || !PipelineStatisticsTable)
    {
        return false;
    }

    BuildPipelineStatisticsRows();

    TSharedPtr<FExpander> StatisticsSection = FExpander::Create(FEditorStyle::MakeExpanderDesc("Pipeline Statistics", PipelineStatisticsTable, false));
    if (!StatisticsSection)
    {
        return false;
    }

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FrameTimeHistogram).SetPadding(FMargin(6, 6, 6, 6));
    Column->AddSlot(PassTable);
    Column->AddSlot(StatisticsSection).SetPadding(FEditorStyle::GetSectionSpacing());

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    TSharedPtr<FVerticalBox> Root = FVerticalBox::Create();
    Root->AddSlot(ToolBar).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Root->AddSlot(FEditorStyle::MakeInnerFrame(ScrollBox)).SetFillCoefficient(1.0f);

    Content = Root;
    return true;
}

void FEditorGPUProfilerPanel::BuildPipelineStatisticsRows()
{
    PipelineStatisticsValues.Reserve(static_cast<int32>(ARRAY_COUNT(GPipelineStatisticsCounters)));

    for (const FPipelineStatisticsCounter& Counter : GPipelineStatisticsCounters)
    {
        TSharedPtr<FTextBlock> Value = CreateValueText("0");

        PipelineStatisticsTable->AddRow(Counter.Label, Value);
        PipelineStatisticsValues.Emplace(Value);
    }
}

TSharedPtr<FToolBar> FEditorGPUProfilerPanel::BuildToolBar()
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

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Enable").SetToolTipText("Issues the timestamp queries the per-pass timings come from"),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetProfilingEnabled(State == ECheckBoxState::Checked);
        }));

    IGPUProfiler* Profiler = GetGPUProfiler();
    const ECheckBoxState StatisticsState = Profiler && Profiler->IsPipelineStatisticsEnabled()
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Pipeline Statistics"), StatisticsState,
        FOnCheckStateChanged::CreateLambda([](ECheckBoxState State)
        {
            IGPUProfiler* Target = GetGPUProfiler();
            if (!Target)
            {
                return;
            }

            if (State == ECheckBoxState::Checked)
            {
                Target->EnablePipelineStatistics();
            }
            else
            {
                Target->DisablePipelineStatistics();
            }
        }));

    Bar->AddSeparator();

    Bar->AddButton(FToolBarItemDesc().SetLabel("Reset"), FOnClicked::CreateLambda([this]()
    {
        if (IGPUProfiler* Target = GetGPUProfiler())
        {
            Target->Reset();
        }

        FrameTimeHistogram->Clear();
        LastIngestedCpuFrameIndex = -1;
    }));

    return Bar;
}

void FEditorGPUProfilerPanel::Release()
{
    bIsProfilingRequested = false;
    ApplyProfilerState();

    ToolBar.Reset();
    FrameTimeHistogram.Reset();
    PassTable.Reset();
    PipelineStatisticsTable.Reset();
    PassValues.Clear();
    PipelineStatisticsValues.Clear();

    FEditorPanel::Release();
}

void FEditorGPUProfilerPanel::OnVisibilityChanged(bool bInIsVisible)
{
    FEditorPanel::OnVisibilityChanged(bInIsVisible);

    ApplyProfilerState();
}

void FEditorGPUProfilerPanel::SetProfilingEnabled(bool bEnabled)
{
    if (bIsProfilingRequested == bEnabled)
    {
        return;
    }

    bIsProfilingRequested = bEnabled;
    ApplyProfilerState();
}

void FEditorGPUProfilerPanel::ApplyProfilerState()
{
    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
    {
        return;
    }

    if (bIsProfilingRequested && IsVisible())
    {
        Profiler->Enable();
    }
    else
    {
        Profiler->Disable();
    }
}

void FEditorGPUProfilerPanel::Tick(float /*DeltaTime*/)
{
    if (!IsVisible())
    {
        return;
    }

    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
    {
        return;
    }

    RefreshFrameTime(*Profiler);
    RefreshPasses(*Profiler);
    RefreshPipelineStatistics(*Profiler);
}

void FEditorGPUProfilerPanel::RefreshFrameTime(IGPUProfiler& Profiler)
{
    const int32 Count = Profiler.GetStoredFrameCount();

    FProfilerGpuFrame Frame;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        if (!Profiler.GetStoredFrame(Index, Frame))
        {
            continue;
        }

        if (Frame.CpuFrameIndex <= LastIngestedCpuFrameIndex && LastIngestedCpuFrameIndex >= 0)
        {
            continue;
        }

        FrameTimeHistogram->AddSample(Frame.GpuMilliseconds);
        LastIngestedCpuFrameIndex = Frame.CpuFrameIndex;
    }
}

void FEditorGPUProfilerPanel::RefreshPasses(IGPUProfiler& Profiler)
{
    FProfilerGpuFrame Latest;
    if (!Profiler.GetLatestFrame(Latest))
    {
        return;
    }

    if (Latest.Intervals.Size() != PassValues.Size())
    {
        PassTable->ClearRows();
        PassValues.Clear();

        PassTable->AddHeaderRow("Pass  (inclusive / exclusive ms)");
        for (const FGPUProfilerInterval& Interval : Latest.Intervals)
        {
            const String Name = Interval.Name ? Interval.Name : "<unnamed>";

            TSharedPtr<FTextBlock> Value = CreateValueText("-");
            PassTable->AddRow(Name, Value);
            PassValues.Add(Name, Value);
        }
    }

    for (const FGPUProfilerInterval& Interval : Latest.Intervals)
    {
        const String Name = Interval.Name ? Interval.Name : "<unnamed>";

        TSharedPtr<FTextBlock>* Value = PassValues.Find(Name);
        if (!Value)
        {
            continue;
        }

        (*Value)->SetText(String::Printf("%.3f  %.3f",
            Time::ToMilliseconds<float>(static_cast<float>(Interval.InclusiveNanoseconds)),
            Time::ToMilliseconds<float>(static_cast<float>(Interval.ExclusiveNanoseconds))));
    }
}

void FEditorGPUProfilerPanel::RebuildPassTable()
{
}

void FEditorGPUProfilerPanel::RefreshPipelineStatistics(IGPUProfiler& Profiler)
{
    if (!Profiler.IsPipelineStatisticsEnabled())
    {
        return;
    }

    FProfilerGpuFrame      Latest;
    FRHIPipelineStatistics Statistics = {};

    if (Profiler.GetLatestFrame(Latest))
    {
        for (const FGPUProfilerInterval& Interval : Latest.Intervals)
        {
            if (!Interval.bHasPipelineStats)
            {
                continue;
            }

            Statistics.IAVertices    += Interval.PipelineStats.IAVertices;
            Statistics.IAPrimitives  += Interval.PipelineStats.IAPrimitives;
            Statistics.VSInvocations += Interval.PipelineStats.VSInvocations;
            Statistics.GSInvocations += Interval.PipelineStats.GSInvocations;
            Statistics.GSPrimitives  += Interval.PipelineStats.GSPrimitives;
            Statistics.CInvocations  += Interval.PipelineStats.CInvocations;
            Statistics.CPrimitives   += Interval.PipelineStats.CPrimitives;
            Statistics.PSInvocations += Interval.PipelineStats.PSInvocations;
            Statistics.HSInvocations += Interval.PipelineStats.HSInvocations;
            Statistics.DSInvocations += Interval.PipelineStats.DSInvocations;
            Statistics.CSInvocations += Interval.PipelineStats.CSInvocations;
            Statistics.ASInvocations += Interval.PipelineStats.ASInvocations;
            Statistics.MSInvocations += Interval.PipelineStats.MSInvocations;
            Statistics.MSPrimitives  += Interval.PipelineStats.MSPrimitives;
        }
    }

    for (int32 Index = 0; Index < PipelineStatisticsValues.Size(); ++Index)
    {
        const uint64 Counter = Statistics.*GPipelineStatisticsCounters[Index].Member;
        PipelineStatisticsValues[Index]->SetText(String::Printf("%llu", static_cast<unsigned long long>(Counter)));
    }
}

TSharedPtr<FTextBlock> FEditorGPUProfilerPanel::CreateValueText(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Monospace;

    return FTextBlock::Create(Desc);
}
