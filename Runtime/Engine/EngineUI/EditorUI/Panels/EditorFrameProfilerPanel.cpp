#include "Engine/EngineUI/EditorUI/Panels/EditorFrameProfilerPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/Histogram.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/ToolBar.h"
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Time/Time.h"

static float ResolveMinimum(float Value)
{
    return Value == TNumericLimits<float>::Max() ? 0.0f : Value;
}

static float ResolveMaximum(float Value)
{
    return Value == TNumericLimits<float>::Lowest() ? 0.0f : Value;
}

FEditorFrameProfilerPanel::FEditorFrameProfilerPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "FrameProfiler", "Frame Profiler")
    , ToolBar(nullptr)
    , FrameTimeHistogram(nullptr)
    , ThreadsColumn(nullptr)
    , ThreadSections()
    , ThreadInfos()
    , LastFrameTimeSample(-1)
    , bIsProfilingRequested(true)
{
}

FEditorFrameProfilerPanel::~FEditorFrameProfilerPanel()
{
}

bool FEditorFrameProfilerPanel::Initialize()
{
    ToolBar = BuildToolBar();
    if (!ToolBar)
    {
        return false;
    }

    FHistogram::FDesc HistogramDesc;
    HistogramDesc.Capacity        = NUM_PROFILER_SAMPLES;
    HistogramDesc.Font            = FEditorStyle::GetFonts().Body;
    HistogramDesc.Label           = "CPU Frame Time (ms)";
    HistogramDesc.PreferredHeight = 80;

    FrameTimeHistogram = FHistogram::Create(HistogramDesc);
    if (!FrameTimeHistogram)
    {
        return false;
    }

    ThreadsColumn = FVerticalBox::Create();

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FrameTimeHistogram).SetPadding(FMargin(6, 6, 6, 6));
    Column->AddSlot(ThreadsColumn);

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

TSharedPtr<FToolBar> FEditorFrameProfilerPanel::BuildToolBar()
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

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Enable").SetToolTipText("Records the traced scopes the per-thread tables are filled from"),
        ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            SetProfilingEnabled(State == ECheckBoxState::Checked);
        }));

    Bar->AddSeparator();

    Bar->AddButton(FToolBarItemDesc().SetLabel("Reset"), FOnClicked::CreateLambda([this]()
    {
        FFrameProfiler::Get().Reset();

        FrameTimeHistogram->Clear();
        LastFrameTimeSample = -1;
    }));

    return Bar;
}

void FEditorFrameProfilerPanel::Release()
{
    bIsProfilingRequested = false;
    ApplyProfilerState();

    ToolBar.Reset();
    FrameTimeHistogram.Reset();
    ThreadsColumn.Reset();
    ThreadSections.Clear();
    ThreadInfos.Clear();

    FEditorPanel::Release();
}

void FEditorFrameProfilerPanel::OnVisibilityChanged(bool bInIsVisible)
{
    FEditorPanel::OnVisibilityChanged(bInIsVisible);

    ApplyProfilerState();
}

void FEditorFrameProfilerPanel::SetProfilingEnabled(bool bEnabled)
{
    if (bIsProfilingRequested == bEnabled)
    {
        return;
    }

    bIsProfilingRequested = bEnabled;
    ApplyProfilerState();
}

void FEditorFrameProfilerPanel::ApplyProfilerState()
{
    if (bIsProfilingRequested && IsVisible())
    {
        FFrameProfiler::Get().Enable();
    }
    else
    {
        FFrameProfiler::Get().Disable();
    }
}

void FEditorFrameProfilerPanel::Tick(float /*DeltaTime*/)
{
    if (!IsVisible())
    {
        return;
    }

    RefreshFrameTime();
    RefreshThreads();
}

void FEditorFrameProfilerPanel::RefreshFrameTime()
{
    const FFrameProfilerFunctionInfo& FrameTime = FFrameProfiler::Get().GetCPUFrameTime();
    if (FrameTime.SampleCount < 1 || FrameTime.CurrentSample == LastFrameTimeSample)
    {
        return;
    }

    LastFrameTimeSample = FrameTime.CurrentSample;

    const int32 NewestSample = (FrameTime.CurrentSample + NUM_PROFILER_SAMPLES - 1) % NUM_PROFILER_SAMPLES;
    FrameTimeHistogram->AddSample(FrameTime.Samples[NewestSample]);
}

void FEditorFrameProfilerPanel::RefreshThreads()
{
    FFrameProfiler::Get().GetFunctionInfo(ThreadInfos);

    if (ThreadInfos.Size() != ThreadSections.Size())
    {
        ThreadsColumn->ClearSlots();
        ThreadSections.Clear();
        ThreadSections.Reserve(ThreadInfos.Size());

        for (int32 ThreadIndex = 0; ThreadIndex < ThreadInfos.Size(); ++ThreadIndex)
        {
            const String Label       = ResolveThreadName(ThreadInfos[ThreadIndex], ThreadIndex);
            const bool   bIsExpanded = FThreadManager::Get().IsMainThread(ThreadInfos[ThreadIndex].ThreadHandle);

            FThreadSection ThreadSection;
            ThreadSection.Table   = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
            ThreadSection.Section = FExpander::Create(FEditorStyle::MakeExpanderDesc(Label, ThreadSection.Table, bIsExpanded));

            ThreadsColumn->AddSlot(ThreadSection.Section);
            ThreadSections.Emplace(ThreadSection);
        }
    }

    for (int32 ThreadIndex = 0; ThreadIndex < ThreadSections.Size(); ++ThreadIndex)
    {
        const FFrameProfilerThreadInfo& ThreadInfo = ThreadInfos[ThreadIndex];

        FThreadSection& ThreadSection = ThreadSections[ThreadIndex];
        ThreadSection.Section->SetLabel(ResolveThreadName(ThreadInfo, ThreadIndex));

        if (ThreadInfo.FunctionInfoMap.Size() != ThreadSection.Values.Size())
        {
            RebuildThreadRows(ThreadSection, ThreadInfo);
        }

        for (auto Scope : ThreadInfo.FunctionInfoMap)
        {
            TSharedPtr<FTextBlock>* Value = ThreadSection.Values.Find(Scope.First);
            if (!Value)
            {
                continue;
            }

            const float Average = Time::ToMilliseconds<float>(Scope.Second.GetAverage());
            const float Minimum = Time::ToMilliseconds<float>(ResolveMinimum(Scope.Second.Min));
            const float Maximum = Time::ToMilliseconds<float>(ResolveMaximum(Scope.Second.Max));

            (*Value)->SetText(String::Printf("%.3f  %.3f  %.3f  %d", Average, Minimum, Maximum, Scope.Second.TotalCalls));
        }
    }
}

void FEditorFrameProfilerPanel::RebuildThreadRows(FThreadSection& ThreadSection, const FFrameProfilerThreadInfo& ThreadInfo)
{
    ThreadSection.Table->ClearRows();
    ThreadSection.Values.Clear();

    ThreadSection.Table->AddHeaderRow("Scope  (avg / min / max in ms, calls)");

    for (auto Scope : ThreadInfo.FunctionInfoMap)
    {
        TSharedPtr<FTextBlock> Value = CreateValueText("-");

        ThreadSection.Table->AddRow(Scope.First, Value).ToolTipText = "Average, minimum and maximum time in the scope, in milliseconds, and how many times it was entered";
        ThreadSection.Values.Add(Scope.First, Value);
    }
}

String FEditorFrameProfilerPanel::ResolveThreadName(const FFrameProfilerThreadInfo& ThreadInfo, int32 ThreadIndex)
{
    if (FThreadManager::Get().IsMainThread(ThreadInfo.ThreadHandle))
    {
        return "Main Thread";
    }

    if (IPlatformThread* Thread = FThreadManager::Get().GetThreadFromHandle(ThreadInfo.ThreadHandle))
    {
        if (!Thread->GetName().IsEmpty())
        {
            return Thread->GetName();
        }
    }

    return String::Printf("Thread %d", ThreadIndex);
}

TSharedPtr<FTextBlock> FEditorFrameProfilerPanel::CreateValueText(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Monospace;

    return FTextBlock::Create(Desc);
}
