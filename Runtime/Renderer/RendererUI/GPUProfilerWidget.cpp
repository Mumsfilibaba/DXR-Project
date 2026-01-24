#include "Core/Time/ElapsedTime.h"
#include "Core/Misc/ConsoleManager.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Renderer/RendererUI/GPUProfilerWidget.h"

static TAutoConsoleVariable<bool> CVarDrawGPUProfiler(
    "Renderer.DrawGPUProfiler",
    "Enables the profiling on the GPU and displays the GPU Profiler window", 
    false);

FGPUProfilerWidget::FGPUProfilerWidget()
    : Samples()
    , ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FGPUProfilerWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FGPUProfilerWidget::~FGPUProfilerWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FGPUProfilerWidget::Draw()
{
    if (CVarDrawGPUProfiler.GetValue())
    {
        DrawWindow();
    }
}

void FGPUProfilerWidget::DrawGPUData(float Width)
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
            ImGui::Text("%s", *Sample.First);

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

void FGPUProfilerWidget::DrawWindow()
{
    const ImVec2 Size     = ImGuiExtensions::GetMainViewportSize();
    const ImVec2 Position = ImGuiExtensions::GetMainViewportPos();

    const float Width  = Math::Clamp<float>(Size.x * 0.6f, 384.0f, 1152.0f);
    const float Height = Math::Clamp<float>(Size.y * 0.5f, 320.0f, 960.0f);

    ImGui::SetNextWindowPos(ImVec2(Size.x * 0.5f, Size.y * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings;

    bool bDrawProfiler = CVarDrawGPUProfiler.GetValue();
    if (ImGui::Begin("GPU Profiler", &bDrawProfiler, Flags))
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

    ImGui::End();

    CVarDrawGPUProfiler->SetAsBool(bDrawProfiler, EConsoleVariableFlags::SetByCode);
}
