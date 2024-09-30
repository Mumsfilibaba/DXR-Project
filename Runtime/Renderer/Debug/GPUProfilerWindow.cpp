#include "GPUProfilerWindow.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Misc/ConsoleManager.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

static TAutoConsoleVariable<bool> CVarDrawGPUProfiler(
    "Renderer.DrawGPUProfiler",
    "Enables the profiling on the GPU and displays the GPU Profiler window", 
    false);

FGPUProfilerWindow::FGPUProfilerWindow()
    : Samples()
    , ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FGPUProfilerWindow::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FGPUProfilerWindow::~FGPUProfilerWindow()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FGPUProfilerWindow::Draw()
{
    if (CVarDrawGPUProfiler.GetValue())
    {
        DrawWindow();
    }
}

void FGPUProfilerWindow::DrawGPUData(float Width)
{
    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("Frame Statistics", 1, TableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        const FGPUProfileSample& GPUFrameTime = FGPUProfiler::Get().GetGPUFrameTime();

        float Avg = GPUFrameTime.GetAverage();
        float Min = GPUFrameTime.Min;
        if (Min == TNumericLimits<float>::Max())
        {
            Min = 0.0f;
        }

        float Max = GPUFrameTime.Max;
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

        ImGui::PlotHistogram("", GPUFrameTime.Samples.Data(), GPUFrameTime.SampleCount, GPUFrameTime.CurrentSample, nullptr, 0.0f, GetMaxLimit(Avg), ImVec2(Width * 0.9825f, 80.0f));

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

    if (ImGui::BeginTable("RenderPasses", 4, TableFlags))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Avg");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        FGPUProfiler::Get().GetGPUSamples(Samples);
        for (auto Sample : Samples)
        {
            ImGui::TableNextRow();

            const float Avg = Sample.Second.GetAverage();
            const float Min = Sample.Second.Min;
            const float Max = Sample.Second.Max;

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Sample.First.GetCString());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.4f ms", TimeUtilities::ToMilliseconds<float>(Avg));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.4f ms", TimeUtilities::ToMilliseconds<float>(Min));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.4f ms", TimeUtilities::ToMilliseconds<float>(Max));
        }

        Samples.Clear();

        ImGui::EndTable();
    }
}

void FGPUProfilerWindow::DrawWindow()
{
    // Draw DebugWindow with DebugStrings
    const ImVec2 DisplaySize = ImGuiExtensions::GetDisplaySize();
    const float Width  = FMath::Max<float>(DisplaySize.x * 0.6f, 400.0f);
    const float Height = DisplaySize.y * 0.75f;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    const ImGuiStyle& Style = ImGui::GetStyle();
    ImVec4 WindowBG = Style.Colors[ImGuiCol_WindowBg];
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ WindowBG.x, WindowBG.y, WindowBG.z, 0.8f });

    ImGui::SetNextWindowPos(ImVec2(DisplaySize.x * 0.5f, DisplaySize.y * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    bool bTempDrawProfiler = CVarDrawGPUProfiler.GetValue();
    if (ImGui::Begin("GPU Profiler", &bTempDrawProfiler, Flags))
    {
        if (ImGui::Button("Start Profile"))
        {
            FGPUProfiler::Get().Enable();
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop Profile"))
        {
            FGPUProfiler::Get().Disable();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            FGPUProfiler::Get().Reset();
        }

        DrawGPUData(Width);

        ImGui::Separator();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::End();

    CVarDrawGPUProfiler->SetAsBool(bTempDrawProfiler, EConsoleVariableFlags::SetByCode);
}
