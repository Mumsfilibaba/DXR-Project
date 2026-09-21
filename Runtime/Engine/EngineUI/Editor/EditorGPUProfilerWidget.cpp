#include "Core/Math/Math.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Time/Time.h"
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
    : ImGuiDelegateHandle()
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

    TRACE_SCOPE("GPU Profiler");

    DrawWindow();
}

void FEditorGPUProfilerWidget::DrawGPUData()
{
    IGPUProfiler* Profiler = GetGPUProfiler();
    if (!Profiler)
        return;

    FProfilerGpuFrame Latest;
    Profiler->GetLatestFrame(Latest);

    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("Frame Statistics", 1, TableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        FProfilerGpuFrame HistogramLatest;

        float Avg = 0.0f;
        float Min = 0.0f;
        float Max = 0.0f;

        TArray<float> HistogramSamples;
        if (Profiler->GetLatestFrame(HistogramLatest))
        {
            Avg = HistogramLatest.GpuMilliseconds;
            Min = Avg;
            Max = Avg;
        }

        const int32 Stored = Profiler->GetStoredFrameCount();

        FProfilerGpuFrame StoredFrame;
        for (int32 Index = 0; Index < Stored; ++Index)
        {
            if (!Profiler->GetStoredFrame(Index, StoredFrame))
            {
                continue;
            }

            HistogramSamples.Add(StoredFrame.GpuMilliseconds);

            Min = (Index == 0) ? StoredFrame.GpuMilliseconds : Math::Min(Min, StoredFrame.GpuMilliseconds);
            Max = Math::Max(Max, StoredFrame.GpuMilliseconds);
            Avg = StoredFrame.GpuMilliseconds;
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
        ImGui::PlotHistogram("", HistogramSamples.IsEmpty() ? nullptr : HistogramSamples.Data(), HistogramSamples.Size(), 0, nullptr, 0.0f, GetMaxLimit(Avg), ImVec2(Width * 0.98f, 80.0f));

        ImGui::EndTable();
    }

    if (ImGui::BeginTable("RenderPasses", 4, TableFlags))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Avg");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        for (const FGPUProfilerInterval& Interval : Latest.Intervals)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Interval.Name ? Interval.Name : "<unnamed>");

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.4f ms", Time::ToMilliseconds<float>(static_cast<float>(Interval.InclusiveNanoseconds)));

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.4f ms", Time::ToMilliseconds<float>(static_cast<float>(Interval.ExclusiveNanoseconds)));

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", Interval.Depth);
        }

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

    const int32 RawLen        = static_cast<int32>(strlen(Raw));
    const int32 NumSeparators = (RawLen - 1) / 3;
    const int32 FinalLen      = RawLen + NumSeparators;

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
        if (EditorWidgets::DrawButton("Stop Statistics"))
        {
            Profiler->DisablePipelineStatistics();
        }
    }
    else
    {
        if (EditorWidgets::DrawButton("Start Statistics"))
        {
            Profiler->EnablePipelineStatistics();
        }
    }

    ImGui::SameLine();

    if (EditorWidgets::DrawButton("Reset Statistics"))
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

    FProfilerGpuFrame Latest;
    FRHIPipelineStatistics Stats = {};
    if (Profiler->GetLatestFrame(Latest))
    {
        for (const FGPUProfilerInterval& Interval : Latest.Intervals)
        {
            if (!Interval.bHasPipelineStats)
            {
                continue;
            }

            Stats.IAVertices    += Interval.PipelineStats.IAVertices;
            Stats.IAPrimitives  += Interval.PipelineStats.IAPrimitives;
            Stats.VSInvocations += Interval.PipelineStats.VSInvocations;
            Stats.GSInvocations += Interval.PipelineStats.GSInvocations;
            Stats.GSPrimitives  += Interval.PipelineStats.GSPrimitives;
            Stats.CInvocations  += Interval.PipelineStats.CInvocations;
            Stats.CPrimitives   += Interval.PipelineStats.CPrimitives;
            Stats.PSInvocations += Interval.PipelineStats.PSInvocations;
            Stats.HSInvocations += Interval.PipelineStats.HSInvocations;
            Stats.DSInvocations += Interval.PipelineStats.DSInvocations;
            Stats.CSInvocations += Interval.PipelineStats.CSInvocations;
            Stats.ASInvocations += Interval.PipelineStats.ASInvocations;
            Stats.MSInvocations += Interval.PipelineStats.MSInvocations;
            Stats.MSPrimitives  += Interval.PipelineStats.MSPrimitives;
        }
    }

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

        Row("IA Vertices",          Stats.IAVertices,    Stats.IAVertices,    Stats.IAVertices);
        Row("IA Primitives",        Stats.IAPrimitives,  Stats.IAPrimitives,  Stats.IAPrimitives);
        Row("VS Invocations",       Stats.VSInvocations, Stats.VSInvocations, Stats.VSInvocations);
        Row("GS Invocations",       Stats.GSInvocations, Stats.GSInvocations, Stats.GSInvocations);
        Row("GS Primitives",        Stats.GSPrimitives,  Stats.GSPrimitives,  Stats.GSPrimitives);
        Row("Clipping Invocations", Stats.CInvocations,  Stats.CInvocations,  Stats.CInvocations);
        Row("Clipping Primitives",  Stats.CPrimitives,   Stats.CPrimitives,   Stats.CPrimitives);
        Row("PS Invocations",       Stats.PSInvocations, Stats.PSInvocations, Stats.PSInvocations);
        Row("HS Invocations",       Stats.HSInvocations, Stats.HSInvocations, Stats.HSInvocations);
        Row("DS Invocations",       Stats.DSInvocations, Stats.DSInvocations, Stats.DSInvocations);
        Row("CS Invocations",       Stats.CSInvocations, Stats.CSInvocations, Stats.CSInvocations);
        Row("AS Invocations",       Stats.ASInvocations, Stats.ASInvocations, Stats.ASInvocations);
        Row("MS Invocations",       Stats.MSInvocations, Stats.MSInvocations, Stats.MSInvocations);
        Row("MS Primitives",        Stats.MSPrimitives,  Stats.MSPrimitives,  Stats.MSPrimitives);

        ImGui::EndTable();
    }
}

void FEditorGPUProfilerWidget::DrawWindow()
{
    IGPUProfiler* Profiler = GetGPUProfiler();

    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoFocusOnAppearing;

    if (EditorWidgets::BeginEditorWindow("GPU Profiler", &bVisible, Flags))
    {
        if (Profiler)
        {
            if (EditorWidgets::DrawButton("Start Profile"))
            {
                Profiler->Enable();
            }

            ImGui::SameLine();

            if (EditorWidgets::DrawButton("Stop Profile"))
            {
                Profiler->Disable();
            }

            ImGui::SameLine();

            if (EditorWidgets::DrawButton("Reset"))
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

    EditorWidgets::EndEditorWindow();
}
