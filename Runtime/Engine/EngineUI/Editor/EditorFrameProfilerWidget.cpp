#include "EditorFrameProfilerWidget.h"
#include "Core/Time/ElapsedTime.h"
#include "Core/Threading/ThreadManager.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"


FEditorFrameProfilerWidget::FEditorFrameProfilerWidget()
    : ThreadInfos()
    , ImGuiDelegateHandle()
    , bVisible(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorFrameProfilerWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorFrameProfilerWidget::~FEditorFrameProfilerWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorFrameProfilerWidget::Draw()
{
    if (bVisible)
    {
        DrawWindow();
    }
}


void FEditorFrameProfilerWidget::DrawCPUData(float Width)
{
    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if (ImGui::BeginTable("Frame Statistics", 1, TableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        const FFrameProfilerFunctionInfo& CPUFrameTime = FFrameProfiler::Get().GetCPUFrameTime();

        float Avg = CPUFrameTime.GetAverage();
        float Min = CPUFrameTime.Min;
        if (Min == TNumericLimits<float>::Max())
        {
            Min = 0.0f;
        }

        float Max = CPUFrameTime.Max;
        if (Max == TNumericLimits<float>::Lowest())
        {
            Max = 0.0f;
        }

        ImGui::Text("FrameTime:");
        ImGui::SameLine();

        ImGui::Text("Avg: %.4f ms", Avg);
        ImGui::SameLine();

        ImGui::Text("Min: %.4f ms", Min);
        ImGui::SameLine();

        ImGui::Text("Max: %.4f ms", Max);

        ImGui::NewLine();

        const auto GetMaxLimit = [](float Num)
        {
            if (Num < 0.01f)
            {
                return 0.01f;
            }
            else if (Num < 0.1f)
            {
                return 0.1f;
            }
            else if (Num < 1.0f)
            {
                return 1.0f;
            }
            else if (Num < 10.0f)
            {
                return 10.0f;
            }
            else if (Num < 100.0f)
            {
                return 100.0f;
            }
            else
            {
                return 1000.0f;
            }
        };

        ImGui::PlotHistogram("", CPUFrameTime.Samples.Data(), CPUFrameTime.SampleCount, CPUFrameTime.CurrentSample, nullptr, 0.0f, GetMaxLimit(Avg), ImVec2(Width * 0.9825f, 80.0f));

        ImGui::EndTable();
    }

    constexpr int32 NumColumns           = 5;
    constexpr int32 NumColumnsExceptName = NumColumns - 1;

    constexpr float NameColumnWidthPercentage  = 0.4f;
    constexpr float OtherColumnWidthPercentage = 1.0f - NameColumnWidthPercentage;

    const float NameColumnWidth  = Width * NameColumnWidthPercentage;
    const float OtherColumnWidth = (Width * OtherColumnWidthPercentage) / static_cast<float>(NumColumnsExceptName);

    FFrameProfiler::Get().GetFunctionInfo(ThreadInfos);

    int32 ThreadIndex = 0;
    for (const FFrameProfilerThreadInfo& ThreadInfo : ThreadInfos)
    {
        bool bIsMainThread = false;

        String ThreadName;
        if (FThreadManager::Get().IsMainThread(ThreadInfo.ThreadHandle))
        {
            ThreadName = "MainThread";
            bIsMainThread = true;
        }
        else
        {
            FGenericPlatformThread* Thread = FThreadManager::Get().GetThreadFromHandle(ThreadInfo.ThreadHandle);
            if (!Thread)
            {
                continue;
            }

            ThreadName = Thread->GetName();
            if (ThreadName.IsEmpty())
            {
                ThreadName = String::Printf("Thread %d", ThreadIndex);
            }
        }

        const ImGuiTreeNodeFlags TreeFlags = bIsMainThread ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;
        if (ImGui::CollapsingHeader(*ThreadName, TreeFlags))
        {
            if (ImGui::BeginTable("Functions", NumColumns, TableFlags | ImGuiTableFlags_Resizable))
            {
                constexpr ImGuiTableColumnFlags ColumnFlags = ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort;
                ImGui::TableSetupColumn("Trace Name", ColumnFlags, NameColumnWidth);
                ImGui::TableSetupColumn("Total Calls", ColumnFlags, OtherColumnWidth);
                ImGui::TableSetupColumn("Avg", ColumnFlags, OtherColumnWidth);
                ImGui::TableSetupColumn("Min", ColumnFlags, OtherColumnWidth);
                ImGui::TableSetupColumn("Max", ColumnFlags, OtherColumnWidth);

                ImGui::TableHeadersRow();

                for (auto Sample : ThreadInfo.FunctionInfoMap)
                {
                    ImGui::TableNextRow();

                    constexpr float MillisecondMultiplier = 1.0f / (1000.0f * 1000.0f);
                    float Avg   = Sample.Second.GetAverage() * MillisecondMultiplier;
                    float Min   = Sample.Second.Min * MillisecondMultiplier;
                    float Max   = Sample.Second.Max * MillisecondMultiplier;
                    int32 Calls = Sample.Second.TotalCalls;

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", *Sample.First);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", Calls);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("Avg: %.4f ms", Avg);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("Min: %.4f ms", Min);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("Max: %.4f ms", Max);
                }

                ImGui::EndTable();
            }
        }

        ThreadIndex++;
    }
}

void FEditorFrameProfilerWidget::DrawWindow()
{
    const ImVec2 Size     = ImGuiExtensions::GetMainViewportSize();
    const ImVec2 Position = ImGuiExtensions::GetMainViewportPos();

    const float Width  = Math::Clamp<float>(Size.x * 0.6f, 384.0f, 1152.0f);
    const float Height = Math::Clamp<float>(Size.y * 0.5f, 320.0f, 960.0f);

    ImGui::SetNextWindowPos(ImVec2(Position.x + (Size.x * 0.5f), Position.y + (Size.y * 0.175f)), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Frame Profiler", &bVisible, Flags))
    {
        if (ImGui::Button("Start Profile"))
        {
            FFrameProfiler::Get().Enable();
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop Profile"))
        {
            FFrameProfiler::Get().Disable();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            FFrameProfiler::Get().Reset();
        }

        DrawCPUData(Width);

        ImGui::Separator();
    }

    ImGui::End();
}
