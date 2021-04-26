#include "Profiler.h"
#include "Console.h"

#include "Rendering/DebugUI.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/GPUProfiler.h"

constexpr Float MICROSECONDS     = 1000.0f;
constexpr Float MILLISECONDS     = 1000.0f * 1000.0f;
constexpr Float SECONDS          = 1000.0f * 1000.0f * 1000.0f;
constexpr Float INV_MICROSECONDS = 1.0f / MICROSECONDS;
constexpr Float INV_MILLISECONDS = 1.0f / MILLISECONDS;
constexpr Float INV_SECONDS      = 1.0f / SECONDS;

constexpr Float MAX_FRAMETIME_MS = 1000.0f / 30.0f;

ConsoleVariable gDrawProfiler(EConsoleVariableType::Bool);
ConsoleVariable gDrawFps(EConsoleVariableType::Bool);

struct ProfilerData
{
    TRef<GPUProfiler> GPUProfiler;
    UInt32 CurrentTimeQueryIndex = 0;

    ProfileSample    CPUFrameTime;
    GPUProfileSample GPUFrameTime;

    Clock Clock;
    Int32 Fps        = 0;
    Int32 CurrentFps = 0;
    
    Bool EnableProfiler = true;
    
    std::unordered_map<std::string, ProfileSample> CPUSamples;
    std::unordered_map<std::string, GPUProfileSample> GPUSamples;
};

static ProfilerData gProfilerData;

static void ImGui_PrintTime(Float Num)
{
    if (Num == FLT_MAX || Num == -FLT_MAX)
    {
        ImGui::Text("0.0 s");
    }
    else if (Num < MICROSECONDS)
    {
        ImGui::Text("%.4f ns", Num);
    }
    else if (Num < MICROSECONDS)
    {
        const Float Time = Num * INV_MICROSECONDS;
        ImGui::Text("%.4f qs", Time);
    }
    else if (Num < SECONDS)
    {
        const Float Time = Num * INV_MILLISECONDS;
        ImGui::Text("%.4f ms", Time);
    }
    else
    {
        const Float Time = Num * INV_SECONDS;
        ImGui::Text("%.4f s", Time);
    }
}

static void ImGui_PrintTiming(const Char* Text, Float Num)
{
    ImGui::Text("%s: ", Text);

    ImGui::NextColumn();

    ImGui_PrintTime(Num);
}

static void ImGui_PrintTiming_SameLine(const Char* Text, Float Num)
{
    ImGui::Text("%s: ", Text);
    ImGui::SameLine();
    ImGui_PrintTime(Num);
}

static Float ImGui_ConvertNumber(Float Num)
{
    if (Num < MICROSECONDS)
    {
        return Num;
    }
    else if (Num < MICROSECONDS)
    {
        return Num * INV_MICROSECONDS;
    }
    else if (Num < SECONDS)
    {
        return Num * INV_MILLISECONDS;
    }
    else
    {
        return Num * INV_SECONDS;
    }
}

static Float ImGui_GetMaxLimit(Float Num)
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
    else if (Num < 3.0f)
    {
        return 3.0f;
    }
    else if (Num < 17.0f)
    {
        return 17.0f;
    }
    else if (Num < 34.0f)
    {
        return 34.0f;
    }
    else if (Num < 100.0f)
    {
        return 100.0f;
    }
    else
    {
        return 1000.0f;
    }
}

static void DrawFPS()
{
    const UInt32 WindowWidth = gMainWindow->GetWidth();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(5.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

    ImGui::SetNextWindowPos(ImVec2(Float(WindowWidth), 0.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("FPS Window", nullptr, Flags);

    const std::string FpsStr = std::to_string(gProfilerData.Fps);
    ImGui::Text("%s", FpsStr.c_str());

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

static void DrawCPUProfileData(Float Width)
{
    const ImGuiTableFlags TableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("Frame Statistics", 1, TableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        Float Avg = gProfilerData.CPUFrameTime.GetAverage();
        Float Min = gProfilerData.CPUFrameTime.Min;
        if (Min == FLT_MAX)
        {
            Min = 0.0f;
        }

        Float Max = gProfilerData.CPUFrameTime.Max;
        if (Max == -FLT_MAX)
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

        ImGui::PlotHistogram(
            "",
            gProfilerData.CPUFrameTime.Samples.Data(),
            gProfilerData.CPUFrameTime.SampleCount,
            gProfilerData.CPUFrameTime.CurrentSample,
            nullptr,
            0.0f,
            ImGui_GetMaxLimit(Avg),
            ImVec2(Width * 0.9825f, 80.0f));

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

    if (ImGui::BeginTable("Functions", 5, TableFlags))
    {
        ImGui::TableSetupColumn("Trace Name");
        ImGui::TableSetupColumn("Total Calls");
        ImGui::TableSetupColumn("Avg");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        for (auto& Sample : gProfilerData.CPUSamples)
        {
            ImGui::TableNextRow();

            Float Avg = Sample.second.GetAverage();
            Float Min = Sample.second.Min;
            Float Max = Sample.second.Max;
            Int32 Calls = Sample.second.TotalCalls;

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Sample.first.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", Calls);
            ImGui::TableSetColumnIndex(2);
            ImGui_PrintTime(Avg);
            ImGui::TableSetColumnIndex(3);
            ImGui_PrintTime(Min);
            ImGui::TableSetColumnIndex(4);
            ImGui_PrintTime(Max);
        }

        ImGui::EndTable();
    }
}

static void DrawGPUProfileData(Float Width)
{
    const ImGuiTableFlags TableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("Frame Statistics", 1, TableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        Float Avg = gProfilerData.GPUFrameTime.GetAverage();
        Float Min = gProfilerData.GPUFrameTime.Min;
        if (Min == FLT_MAX)
        {
            Min = 0.0f;
        }

        Float Max = gProfilerData.GPUFrameTime.Max;
        if (Max == -FLT_MAX)
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

        ImGui::PlotHistogram(
            "",
            gProfilerData.GPUFrameTime.Samples.Data(),
            gProfilerData.GPUFrameTime.SampleCount,
            gProfilerData.GPUFrameTime.CurrentSample,
            nullptr,
            0.0f,
            ImGui_GetMaxLimit(Avg),
            ImVec2(Width * 0.9825f, 80.0f));

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

    if (ImGui::BeginTable("Functions", 4, TableFlags))
    {
        ImGui::TableSetupColumn("Trace Name");
        ImGui::TableSetupColumn("Avg");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        for (auto& Sample : gProfilerData.GPUSamples)
        {
            ImGui::TableNextRow();

            Float Avg = Sample.second.GetAverage();
            Float Min = Sample.second.Min;
            Float Max = Sample.second.Max;

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Sample.first.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui_PrintTime(Avg);
            ImGui::TableSetColumnIndex(2);
            ImGui_PrintTime(Min);
            ImGui::TableSetColumnIndex(3);
            ImGui_PrintTime(Max);
        }

        ImGui::EndTable();
    }
}

static void DrawProfiler()
{
    // Draw DebugWindow with DebugStrings
    const UInt32 WindowWidth  = gMainWindow->GetWidth();
    const UInt32 WindowHeight = gMainWindow->GetHeight();
    const Float Width         = Math::Max(WindowWidth * 0.6f, 400.0f);
    const Float Height        = WindowHeight * 0.75f;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    ImGui::SetNextWindowPos(ImVec2(Float(WindowWidth) * 0.5f, Float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoResize           |
        ImGuiWindowFlags_NoCollapse         |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    Bool TempDrawProfiler = gDrawProfiler.GetBool();
    if (ImGui::Begin("Profiler", &TempDrawProfiler, Flags))
    {
        if (ImGui::Button("Start Profile"))
        {
            Profiler::Enable();
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop Profile"))
        {
            Profiler::Disable();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            Profiler::Reset();
        }

        ImGuiTabBarFlags TabBarFlags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("ProfilerTabs", TabBarFlags))
        {
            if (ImGui::BeginTabItem("CPU"))
            {
                DrawCPUProfileData(Width);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("GPU"))
            {
                DrawGPUProfileData(Width);
                ImGui::EndTabItem();
            }
            // TODO: Memory?
            ImGui::EndTabBar();
        }
        ImGui::Separator();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::End();

    gDrawProfiler.SetBool(TempDrawProfiler);
}

void Profiler::Init()
{
    INIT_CONSOLE_VARIABLE("r.DrawFps", gDrawFps);
    gDrawFps.SetBool(false);

    INIT_CONSOLE_VARIABLE("r.DrawProfiler", gDrawProfiler);
    gDrawProfiler.SetBool(false);
}

void Profiler::Tick()
{
    Clock& Clock = gProfilerData.Clock;
    Clock.Tick();

    gProfilerData.CurrentFps++;
    if (Clock.GetTotalTime().AsSeconds() > 1.0f)
    {
        gProfilerData.Fps        = gProfilerData.CurrentFps;
        gProfilerData.CurrentFps = 0;

        Clock.Reset();
    }

    if (gDrawFps.GetBool())
    {
        DebugUI::DrawUI(DrawFPS);
    }

    if (gProfilerData.EnableProfiler)
    {
        const Double Delta = Clock.GetDeltaTime().AsMilliSeconds();
        gProfilerData.CPUFrameTime.AddSample(Float(Delta));

        if (gProfilerData.GPUProfiler)
        {
            TimeQuery Query;
            gProfilerData.GPUProfiler->GetTimeQuery(Query, gProfilerData.GPUFrameTime.TimeQueryIndex);

            Double Frequency = (Double)gProfilerData.GPUProfiler->GetFrequency();
            Double DeltaTime = (Double)(Query.End - Query.Begin);
            Double Duration  = (DeltaTime / Frequency) * 1000.0;
            gProfilerData.GPUFrameTime.AddSample((Float)Duration);
        }
    }

    if (gDrawProfiler.GetBool())
    {
        DebugUI::DrawUI(DrawProfiler);
    }
}

void Profiler::Enable()
{
    gProfilerData.EnableProfiler = true;
}

void Profiler::Disable()
{
    gProfilerData.EnableProfiler = false;
}

void Profiler::Reset()
{
    gProfilerData.CPUFrameTime.Reset();
    gProfilerData.GPUFrameTime.Reset();

    for (auto& Sample : gProfilerData.CPUSamples)
    {
        Sample.second.Reset();
    }

    for (auto& Sample : gProfilerData.GPUSamples)
    {
        Sample.second.Reset();
    }
}

void Profiler::BeginTraceScope(const Char* Name)
{
    if (gProfilerData.EnableProfiler)
    {
        const std::string ScopeName = Name;

        auto Entry = gProfilerData.CPUSamples.find(ScopeName);
        if (Entry == gProfilerData.CPUSamples.end())
        {
            auto NewSample = gProfilerData.CPUSamples.insert(std::make_pair(ScopeName, ProfileSample()));
            NewSample.first->second.Begin();
        }
        else
        {
            Entry->second.Begin();
        }
    }
}

void Profiler::EndTraceScope(const Char* Name)
{
    if (gProfilerData.EnableProfiler)
    {
        const std::string ScopeName = Name;

        auto Entry = gProfilerData.CPUSamples.find(ScopeName);
        if (Entry != gProfilerData.CPUSamples.end())
        {
            Entry->second.End();
        }
        else
        {
            Assert(false);
        }
    }
}

void Profiler::BeginGPUFrame(CommandList& CmdList)
{
    if (gProfilerData.GPUProfiler && gProfilerData.EnableProfiler)
    {
        CmdList.BeginTimeStamp(gProfilerData.GPUProfiler.Get(), gProfilerData.GPUFrameTime.TimeQueryIndex);
    }
}

void Profiler::BeginGPUTrace(CommandList& CmdList, const Char* Name)
{
    if (gProfilerData.GPUProfiler && gProfilerData.EnableProfiler)
    {
        const std::string ScopeName = Name;

        Int32 TimeQueryIndex = -1;

        auto Entry = gProfilerData.GPUSamples.find(ScopeName);
        if (Entry == gProfilerData.GPUSamples.end())
        {
            auto NewSample = gProfilerData.GPUSamples.insert(std::make_pair(ScopeName, GPUProfileSample()));
            NewSample.first->second.TimeQueryIndex = ++gProfilerData.CurrentTimeQueryIndex;
            TimeQueryIndex = NewSample.first->second.TimeQueryIndex;
        }
        else
        {
            TimeQueryIndex = Entry->second.TimeQueryIndex;
        }

        if (TimeQueryIndex >= 0)
        {
            CmdList.BeginTimeStamp(gProfilerData.GPUProfiler.Get(), TimeQueryIndex);
        }
    }
}

void Profiler::EndGPUTrace(CommandList& CmdList, const Char* Name)
{
    if (gProfilerData.GPUProfiler && gProfilerData.EnableProfiler)
    {
        const std::string ScopeName = Name;

        Int32 TimeQueryIndex = -1;

        auto Entry = gProfilerData.GPUSamples.find(ScopeName);
        if (Entry != gProfilerData.GPUSamples.end())
        {
            TimeQueryIndex = Entry->second.TimeQueryIndex;
            CmdList.EndTimeStamp(gProfilerData.GPUProfiler.Get(), TimeQueryIndex);

            if (TimeQueryIndex >= 0)
            {
                TimeQuery Query;
                gProfilerData.GPUProfiler->GetTimeQuery(Query, TimeQueryIndex);

                Double Frequency = (Double)gProfilerData.GPUProfiler->GetFrequency();
                Double Duration  = (Double)((Query.End - Query.Begin) / Frequency) * SECONDS;
                Entry->second.AddSample(Duration);
            }
        }
    }
}

void Profiler::SetGPUProfiler(GPUProfiler* Profiler)
{
    gProfilerData.GPUProfiler = MakeSharedRef<GPUProfiler>(Profiler);
}

const ProfileSample* Profiler::GetSample(const Char* Name)
{
    std::string SampleName = std::string(Name);

    auto Sample = gProfilerData.CPUSamples.find(SampleName);
    if (Sample == gProfilerData.CPUSamples.end())
    {
        LOG_WARNING("No sample found with name '" + SampleName + "'");
        return nullptr;
    }

    return &Sample->second;
}

const GPUProfileSample* Profiler::GetGPUSample(const Char* Name)
{
    std::string SampleName = std::string(Name);

    auto Sample = gProfilerData.GPUSamples.find(SampleName);
    if (Sample == gProfilerData.GPUSamples.end())
    {
        LOG_WARNING("No GPU sample found with name '" + SampleName + "'");
        return nullptr;
    }

    return &Sample->second;
}

const ProfileSample* Profiler::GetFrameTimeSamples()
{
    return &gProfilerData.CPUFrameTime;
}

const GPUProfileSample* Profiler::GetGPUFrameTimeSamples()
{
    return &gProfilerData.GPUFrameTime;
}

void Profiler::EndGPUFrame(CommandList& CmdList)
{
    if (gProfilerData.GPUProfiler && gProfilerData.EnableProfiler)
    {
        CmdList.EndTimeStamp(gProfilerData.GPUProfiler.Get(), gProfilerData.GPUFrameTime.TimeQueryIndex);
    }
}
