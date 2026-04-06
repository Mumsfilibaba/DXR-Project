#include "Core/Time/TimeUtilities.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "Engine/EngineUI/Editor/EditorGPUProfilerWidget.h"

static IGPUProfiler* GetGPUProfiler()
{
    IRendererModule* RendererModule = IRendererModule::Get();
    return RendererModule ? &RendererModule->GetGPUProfiler() : nullptr;
}

FEditorGPUProfilerWidget::FEditorGPUProfilerWidget()
    : Samples()
    , ImGuiDelegateHandle()
    , bVisible(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorGPUProfilerWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorGPUProfilerWidget::~FEditorGPUProfilerWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorGPUProfilerWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    DrawWindow();
}

void FEditorGPUProfilerWidget::DrawGPUData()
{
    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
        return;

    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("Frame Statistics", 1, TableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        const FGPUProfileSample& GPUFrameTime = Profiler->GetGPUFrameTime();

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
                return 0.01f;
            else if (Num < 0.1f)
                return 0.1f;
            else if (Num < 1.0f)
                return 1.0f;
            else if (Num < 10.0f)
                return 10.0f;
            else if (Num < 100.0f)
                return 100.0f;
            else
                return 1000.0f;
        };

        const float Width = ImGui::GetContentRegionAvail().x;
        ImGui::PlotHistogram("", GPUFrameTime.Samples.Data(), GPUFrameTime.SampleCount, GPUFrameTime.CurrentSample, nullptr, 0.0f, GetMaxLimit(Avg), ImVec2(Width * 0.98f, 80.0f));

        ImGui::EndTable();
    }

    if (ImGui::BeginTable("RenderPasses", 4, TableFlags))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Avg");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        Profiler->GetGPUSamples(Samples);
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

static void FormatWithSeparators(CHAR* Buffer, int32 BufferSize, uint64 Value)
{
    if (Value == 0 || Value == TNumericLimits<uint64>::Max())
    {
        snprintf(Buffer, BufferSize, "%llu", Value == 0 ? 0ull : 0ull);
        return;
    }

    CHAR Raw[32];
    snprintf(Raw, sizeof(Raw), "%llu", Value);

    const int32 RawLen = static_cast<int32>(strlen(Raw));
    const int32 NumSeparators = (RawLen - 1) / 3;
    const int32 FinalLen = RawLen + NumSeparators;

    if (FinalLen >= BufferSize)
    {
        snprintf(Buffer, BufferSize, "%llu", Value);
        return;
    }

    int32 WritePos = FinalLen;
    Buffer[WritePos--] = '\0';

    for (int32 i = RawLen - 1, Digit = 0; i >= 0; i--, Digit++)
    {
        if (Digit > 0 && (Digit % 3) == 0)
        {
            Buffer[WritePos--] = ',';
        }
        Buffer[WritePos--] = Raw[i];
    }
}

void FEditorGPUProfilerWidget::DrawPipelineStatistics()
{
    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
        return;

    const bool bStatsEnabled = Profiler->IsPipelineStatisticsEnabled();
    if (bStatsEnabled)
    {
        if (ImGui::Button("Stop Statistics"))
        {
            Profiler->DisablePipelineStatistics();
        }
    }
    else
    {
        if (ImGui::Button("Start Statistics"))
        {
            Profiler->EnablePipelineStatistics();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset Statistics"))
    {
        Profiler->Reset();
    }

    if (!bStatsEnabled)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("Enable pipeline statistics to see data.");
        return;
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Frame Total (aggregated from per-pass)");
    ImGui::Spacing();

    const FRHIPipelineStatistics&     Stats    = Profiler->GetPipelineStatistics();
    const FPipelineStatisticsMinMax&  MinMax   = Profiler->GetPipelineStatisticsMinMax();
    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("PipelineStatistics", 4, TableFlags))
    {
        ImGui::TableSetupColumn("Counter");
        ImGui::TableSetupColumn("Current",  ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Min",      ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Max",      ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        CHAR Buf[48];
        auto Row = [&Buf](const CHAR* Label, uint64 Current, uint64 MinVal, uint64 MaxVal)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(Label);

            ImGui::TableSetColumnIndex(1);
            FormatWithSeparators(Buf, sizeof(Buf), Current);
            ImGui::TextUnformatted(Buf);

            ImGui::TableSetColumnIndex(2);
            if (MinVal == TNumericLimits<uint64>::Max())
                ImGui::TextUnformatted("-");
            else
            {
                FormatWithSeparators(Buf, sizeof(Buf), MinVal);
                ImGui::TextUnformatted(Buf);
            }

            ImGui::TableSetColumnIndex(3);
            if (MaxVal == 0 && MinVal == TNumericLimits<uint64>::Max())
                ImGui::TextUnformatted("-");
            else
            {
                FormatWithSeparators(Buf, sizeof(Buf), MaxVal);
                ImGui::TextUnformatted(Buf);
            }
        };

        Row("IA Vertices",          Stats.IAVertices,    MinMax.Min.IAVertices,    MinMax.Max.IAVertices);
        Row("IA Primitives",        Stats.IAPrimitives,  MinMax.Min.IAPrimitives,  MinMax.Max.IAPrimitives);
        Row("VS Invocations",       Stats.VSInvocations, MinMax.Min.VSInvocations, MinMax.Max.VSInvocations);
        Row("GS Invocations",       Stats.GSInvocations, MinMax.Min.GSInvocations, MinMax.Max.GSInvocations);
        Row("GS Primitives",        Stats.GSPrimitives,  MinMax.Min.GSPrimitives,  MinMax.Max.GSPrimitives);
        Row("Clipping Invocations", Stats.CInvocations,  MinMax.Min.CInvocations,  MinMax.Max.CInvocations);
        Row("Clipping Primitives",  Stats.CPrimitives,   MinMax.Min.CPrimitives,   MinMax.Max.CPrimitives);
        Row("PS Invocations",       Stats.PSInvocations, MinMax.Min.PSInvocations, MinMax.Max.PSInvocations);
        Row("HS Invocations",       Stats.HSInvocations, MinMax.Min.HSInvocations, MinMax.Max.HSInvocations);
        Row("DS Invocations",       Stats.DSInvocations, MinMax.Min.DSInvocations, MinMax.Max.DSInvocations);
        Row("CS Invocations",       Stats.CSInvocations, MinMax.Min.CSInvocations, MinMax.Max.CSInvocations);
        Row("AS Invocations",       Stats.ASInvocations, MinMax.Min.ASInvocations, MinMax.Max.ASInvocations);
        Row("MS Invocations",       Stats.MSInvocations, MinMax.Min.MSInvocations, MinMax.Max.MSInvocations);
        Row("MS Primitives",        Stats.MSPrimitives,  MinMax.Min.MSPrimitives,  MinMax.Max.MSPrimitives);

        ImGui::EndTable();
    }
}

void FEditorGPUProfilerWidget::DrawWindow()
{
    IGPUProfiler* Profiler = GetGPUProfiler();

    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("GPU Profiler", &bVisible, Flags))
    {
        if (Profiler)
        {
            if (ImGui::Button("Start Profile"))
            {
                Profiler->Enable();
            }

            ImGui::SameLine();

            if (ImGui::Button("Stop Profile"))
            {
                Profiler->Disable();
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset"))
            {
                Profiler->Reset();
            }

            if (ImGui::BeginTabBar("GPUProfilerTabs"))
            {
                if (ImGui::BeginTabItem("Timing"))
                {
                    DrawGPUData();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Pipeline Statistics"))
                {
                    DrawPipelineStatistics();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        else
        {
            ImGui::TextDisabled("GPU Profiler not available");
        }
    }

    ImGui::End();
}
