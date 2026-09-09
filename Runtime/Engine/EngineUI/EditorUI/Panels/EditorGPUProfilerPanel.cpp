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

static float ResolveMinimum(float Value)
{
    return Value == TNumericLimits<float>::Max() ? 0.0f : Value;
}

static float ResolveMaximum(float Value)
{
    return Value == TNumericLimits<float>::Lowest() ? 0.0f : Value;
}

FEditorGPUProfilerPanel::FEditorGPUProfilerPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "GPUProfiler", "GPU Profiler")
    , ToolBar(nullptr)
    , FrameTimeHistogram(nullptr)
    , PassTable(nullptr)
    , PipelineStatisticsTable(nullptr)
    , Samples()
    , PassValues()
    , PipelineStatisticsValues()
    , LastFrameTimeSample(-1)
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
    HistogramDesc.Capacity        = NUM_GPU_PROFILER_SAMPLES;
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
    Column->AddSlot(StatisticsSection);

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    TSharedPtr<FVerticalBox> Root = FVerticalBox::Create();
    Root->AddSlot(ToolBar);
    Root->AddSlot(ScrollBox).SetFillCoefficient(1.0f);

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
        LastFrameTimeSample = -1;
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
    Samples.Clear();
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
    const FGPUProfileSample& FrameTime = Profiler.GetGPUFrameTime();
    if (FrameTime.SampleCount < 1 || FrameTime.CurrentSample == LastFrameTimeSample)
    {
        return;
    }

    LastFrameTimeSample = FrameTime.CurrentSample;

    const int32 NewestSample = (FrameTime.CurrentSample + NUM_GPU_PROFILER_SAMPLES - 1) % NUM_GPU_PROFILER_SAMPLES;
    FrameTimeHistogram->AddSample(FrameTime.Samples[NewestSample]);
}

void FEditorGPUProfilerPanel::RefreshPasses(IGPUProfiler& Profiler)
{
    Profiler.GetGPUSamples(Samples);

    if (Samples.Size() != PassValues.Size())
    {
        RebuildPassTable();
    }

    for (auto Sample : Samples)
    {
        TSharedPtr<FTextBlock>* Value = PassValues.Find(Sample.First);
        if (!Value)
        {
            continue;
        }

        const float Average = Time::ToMilliseconds<float>(Sample.Second.GetAverage());
        const float Minimum = Time::ToMilliseconds<float>(ResolveMinimum(Sample.Second.Min));
        const float Maximum = Time::ToMilliseconds<float>(ResolveMaximum(Sample.Second.Max));

        (*Value)->SetText(String::Printf("%.3f  %.3f  %.3f", Average, Minimum, Maximum));
    }
}

void FEditorGPUProfilerPanel::RebuildPassTable()
{
    PassTable->ClearRows();
    PassValues.Clear();

    PassTable->AddHeaderRow("Pass  (avg / min / max in ms)");

    for (auto Sample : Samples)
    {
        TSharedPtr<FTextBlock> Value = CreateValueText("-");

        PassTable->AddRow(Sample.First, Value).ToolTipText = "Average, minimum and maximum GPU time for the pass, in milliseconds";
        PassValues.Add(Sample.First, Value);
    }
}

void FEditorGPUProfilerPanel::RefreshPipelineStatistics(IGPUProfiler& Profiler)
{
    if (!Profiler.IsPipelineStatisticsEnabled())
    {
        return;
    }

    const FRHIPipelineStatistics& Statistics = Profiler.GetPipelineStatistics();

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
