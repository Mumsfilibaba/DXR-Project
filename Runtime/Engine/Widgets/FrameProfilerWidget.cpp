#include "FrameProfilerWidget.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Threading/ThreadManager.h"
#include "Application/Application.h"

static TAutoConsoleVariable<bool> CVarDrawFps(
    "Engine.DrawFps",
    "Enable FPS counter in the top right corner",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarDrawFrameProfiler(
    "Engine.DrawFrameProfiler",
    "Enables the FrameProfiler and displays the profiler window",
    false,
    EConsoleVariableFlags::Default);

void FFrameProfilerWidget::Paint()
{
    if (CVarDrawFps.GetValue())
    {
        DrawFPS();
    }

    if (CVarDrawFrameProfiler.GetValue())
    {
        DrawWindow();
    }
}

void FFrameProfilerWidget::DrawFPS()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(5.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

    const ImVec2 Size     = FImGui::GetMainViewportSize();
    const ImVec2 Position = FImGui::GetMainViewportPos();
    ImGui::SetNextWindowPos(ImVec2(Position.x + Size.x, Position.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("FPS Window", nullptr, Flags);
    ImGui::Text("%d", FFrameProfiler::Get().GetFramesPerSecond());
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void FFrameProfilerWidget::DrawCPUData(float Width)
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

    // TODO: Fix timeline
    //if (ImGui::BeginTable("Threads", 2, TableFlags))
    //{
    //    ImGui::TableSetupColumn("Thread", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    //    ImGui::TableSetupColumn("Timeline");
    //    ImGui::TableHeadersRow();

    //    ImGui::TableNextRow();

    //    ImGui::TableSetColumnIndex(0);
    //    ImGui::Text("Main Thread");

    //    ImGui::TableSetColumnIndex(1);

    //    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    //    ImGui::Button("Thing 1", ImVec2(50.0f, 20.0f));
    //    if (ImGui::IsItemHovered())
    //    {
    //        ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
    //    }

    //    ImGui::SameLine();

    //    ImGui::Button("Thing 2", ImVec2(30.0f, 20.0f));
    //    if (ImGui::IsItemHovered())
    //    {
    //        ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
    //    }

    //    ImGui::SameLine();
    //    ImGui::Button("Thing 3", ImVec2(70.0f, 20.0f));
    //    if (ImGui::IsItemHovered())
    //    {
    //        ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
    //    }

    //    ImGui::SameLine();
    //    ImGui::Button("Thing 4", ImVec2(20.0f, 20.0f));
    //    if (ImGui::IsItemHovered())
    //    {
    //        ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
    //    }

    //    ImGui::Dummy(ImVec2(40.0f, 20.0f));
    //    ImGui::SameLine();

    //    ImGui::Button("Thing 4", ImVec2(70.0f, 20.0f));
    //    if (ImGui::IsItemHovered())
    //    {
    //        ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
    //    }

    //    ImGui::PopStyleVar();

    //    ImGui::EndTable();
    //}

    // Retrieve a copy of the CPU samples
    FFrameProfiler::Get().GetFunctionInfo(ThreadInfos);

    int32 ThreadIndex = 0;
    for (const FFrameProfilerThreadInfo& ThreadInfo : ThreadInfos)
    {
        FString ThreadName;
        if (FThreadManager::Get().IsMainThread(ThreadInfo.ThreadHandle))
        {
            ThreadName = "MainThread";
        }
        else
        {
            FGenericThread* Thread = FThreadManager::Get().GetThreadFromHandle(ThreadInfo.ThreadHandle);
            if (!Thread)
            {
                continue;
            }

            ThreadName = Thread->GetName();

            if (ThreadName.IsEmpty())
            {
                ThreadName = FString::CreateFormatted("Thread%d", ThreadIndex);
            }
        }

        if (ImGui::CollapsingHeader(ThreadName.GetCString(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("Functions", 5, TableFlags | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Trace Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Total Calls");
                ImGui::TableSetupColumn("Avg");
                ImGui::TableSetupColumn("Min");
                ImGui::TableSetupColumn("Max");
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
                    ImGui::Text("%s", Sample.First.GetCString());
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

void FFrameProfilerWidget::DrawWindow()
{
    // Draw DebugWindow with DebugStrings
	const ImVec2 Size     = FImGui::GetMainViewportSize();
	const ImVec2 Position = FImGui::GetMainViewportPos();
    const float Width  = FMath::Max<float>(Size.x * 0.6f, 400.0f);
    const float Height = Size.y * 0.75f;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    ImGui::SetNextWindowPos(ImVec2(Position.x + (Size.x * 0.5f), Position.y + (Size.y * 0.175f)), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings;

    bool bTempDrawProfiler = CVarDrawFrameProfiler.GetValue();
    if (ImGui::Begin("Profiler", &bTempDrawProfiler, Flags))
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

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::End();

    CVarDrawFrameProfiler->SetAsBool(bTempDrawProfiler, EConsoleVariableFlags::SetByCode);
}
