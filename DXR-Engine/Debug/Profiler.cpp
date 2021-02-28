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

struct ProfileSample
{
    FORCEINLINE void Begin()
    {
        Clock.Tick();
    }

    FORCEINLINE void End()
    {
        Clock.Tick();

        Float Delta = Clock.GetDeltaTime().AsNanoSeconds();
        AddSample(Delta);
    }

    FORCEINLINE void AddSample(Float NewSample)
    {
        Samples[CurrentSample] = NewSample;
        Min = Math::Min(NewSample, Min);
        Max = Math::Max(NewSample, Max);

        CurrentSample++;
        SampleCount = Math::Min<Int32>(Samples.Size(), SampleCount + 1);

        if (CurrentSample >= Int32(Samples.Size()))
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE Float GetAverage() const
    {
        Float Average = 0.0f;
        for (Int32 n = 0; n < SampleCount; n++)
        {
            Average += Samples[n];
        }

        return Average / Float(SampleCount);
    }

    TStaticArray<Float, NUM_PROFILER_SAMPLES> Samples;
    Clock Clock;
    Float Max = -FLT_MAX;
    Float Min = FLT_MAX;
    Int32 SampleCount   = 0;
    Int32 CurrentSample = 0;
};

struct ProfilerData
{
    TRef<GPUProfiler> GPUProfiler;
    ProfileSample     FrameTime;

    Clock Clock;
    Int32 Fps        = 0;
    Int32 CurrentFps = 0;
    
    Bool EnableProfiler = true;
    
    std::unordered_map<std::string, ProfileSample> Samples;
};

static ProfilerData gProfilerData;

static void ImGui_PrintTime(Float Num)
{
    if (Num < MICROSECONDS)
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

//static void ImGui_DrawTextRect(ImDrawList* DrawList, const Char* Text, Float Width, Float Height)
//{
//    const Float Padding = 5.0f;
//
//    const ImVec2 ScreenPos = ImGui::GetCursorScreenPos();
//    ImVec2 UpperLeft  = ImVec2(ScreenPos.x, ScreenPos.y);
//    ImVec2 LowerRight = ImVec2(UpperLeft.x + Width, UpperLeft.y + Height);
//    ImGui::Dummy(ImVec2(Width, Height));
//
//    ImVec2 TextSize = ImGui::CalcTextSize(Text);
//    ImVec2 TextPos;
//    TextPos.x = UpperLeft.x + Padding;
//    TextPos.y = UpperLeft.y + (Height / 2.0f) - (TextSize.y / 2.0f);
//
//    ImColor TitleColor = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);
//    ImColor TextColor  = ImGui::GetStyleColorVec4(ImGuiCol_Text);
//    DrawList->PushClipRect(UpperLeft, LowerRight, true);
//    DrawList->AddRectFilled(UpperLeft, LowerRight, TitleColor);
//    DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), TextPos, TextColor, Text);
//    DrawList->PopClipRect();
//}

static void DrawProfiler()
{
    // Draw DebugWindow with DebugStrings
    const UInt32 WindowWidth  = gMainWindow->GetWidth();
    const UInt32 WindowHeight = gMainWindow->GetHeight();
    const Float Width         = Math::Max(WindowWidth * 0.6f, 400.0f);
    const Float Height        = WindowHeight * 0.75f;

    ImGui::ShowDemoWindow();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
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
        ImGui::Button("Start Profiler");

        const Float ChildWidth  = Width * 0.985f;
        const Float ChildHeight = Height * 0.13f;

        ImGui::BeginChild("##Graph", ImVec2(ChildWidth, ChildHeight), true);

        const Float FtAvg = gProfilerData.FrameTime.GetAverage();
        ImGui::PlotHistogram(
            "",
            gProfilerData.FrameTime.Samples.Data(),
            gProfilerData.FrameTime.SampleCount,
            gProfilerData.FrameTime.CurrentSample,
            nullptr,
            0.0f,
            ImGui_GetMaxLimit(FtAvg),
            ImVec2(ChildWidth, 80.0f));

        ImGui::NewLine();

        ImGui::Text("FPS:");
        ImGui::SameLine();
        ImGui::Text("%d", gProfilerData.Fps);
        ImGui::SameLine();

        ImGui::Text("FrameTime:");
        ImGui::SameLine();
        ImGui::Text("Avg: %.4f ms", FtAvg);

        ImGui::EndChild();

        static Bool Clicked = false;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 0.9f));

        ImGui::BeginChild("##Threads", ImVec2(ChildWidth, 40.0f), true);

        ImGui::Columns(2, 0, false);
        ImGui::SetColumnWidth(0, 100.0f);

        ImGui::Text("Main Thread");
        ImGui::NextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::Button("Thing 1", ImVec2(50.0f, 20.0f));
        if (ImGui::IsItemClicked())
        {
            Clicked = !Clicked;
        }
        else if (ImGui::IsMouseClicked(0))
        {
            Clicked = false;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
        }

        ImGui::SameLine();

        ImGui::Button("Thing 2", ImVec2(30.0f, 20.0f));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
        }

        ImGui::SameLine();
        ImGui::Button("Thing 3", ImVec2(70.0f, 20.0f));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
        }

        ImGui::SameLine();
        ImGui::Button("Thing 4", ImVec2(20.0f, 20.0f));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
        }

        ImGui::Dummy(ImVec2(40.0f, 20.0f));
        ImGui::SameLine();

        ImGui::Button("Thing 4", ImVec2(70.0f, 20.0f));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Start: 0.0 ms\nEnd: 0.5 ms\nDuration: 0.5 ms\n");
        }

        ImGui::PopStyleVar();
        ImGui::Columns(1);

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        if (Clicked)
        {
            ImGui::BeginChild("##Selection", ImVec2(ChildWidth, 45.0f), false);
            if (ImGui::CollapsingHeader("Thing 1"))
            {
                ImGui::BeginChild("##Threads1", ImVec2(ChildWidth, 45.0f), true, ImGuiWindowFlags_AlwaysAutoResize);

                ImGui::Text("FPS:");
                ImGui::SameLine();
                ImGui::Text("%d", gProfilerData.Fps);

                ImGui::Text("FrameTime:");
                ImGui::SameLine();
                ImGui::Text("Avg: %.4f ms", FtAvg);

                ImGui::PlotLines(
                    "",
                    gProfilerData.FrameTime.Samples.Data(),
                    gProfilerData.FrameTime.SampleCount,
                    gProfilerData.FrameTime.CurrentSample,
                    nullptr,
                    0.0f,
                    ImGui_GetMaxLimit(FtAvg),
                    ImVec2(ChildWidth - 2.0f, 60.0f));

                ImGui::EndChild();
            }
            ImGui::EndChild();
        }

        ImGui::BeginChild("##Functions1", ImVec2(ChildWidth, 110.0f), true, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Columns(4);

        ImGui::Text("Name");
        ImGui::NextColumn();
        ImGui::Text("Avg");
        ImGui::NextColumn();
        ImGui::Text("Min");
        ImGui::NextColumn();
        ImGui::Text("Max");
        ImGui::NextColumn();

        ImGui::Separator();

        for (UInt32 i = 0; i < 4; i++)
        {
            ImGui::Text("Thing %d", i + 1);
            ImGui::NextColumn();
            ImGui::Text("%.4f ms", 0.5f);
            ImGui::NextColumn();
            ImGui::Text("%.4f ms", 0.5f);
            ImGui::NextColumn();
            ImGui::Text("%.4f ms", 0.5f);
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        ImGui::EndChild();
        
        //ImGui::BeginChild("##MainThread", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        //if (ImGui::CollapsingHeader("Main Thread"))
        //{
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
        //}

        //ImGui::EndChild();

        //ImGui::BeginChild("##Selected Sample", ImVec2(ChildWidth, ChildHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

        //ImGui::Text("FrameTime:");
        //ImGui::SameLine();
        //ImGui::Text("Avg: %.4f ms", FtAvg);

        //ImGui::PlotLines(
        //    "",
        //    gProfilerData.FrameTime.Samples.Data(),
        //    gProfilerData.FrameTime.SampleCount,
        //    gProfilerData.FrameTime.CurrentSample,
        //    nullptr,
        //    0.0f,
        //    ImGui_GetMaxLimit(FtAvg),
        //    ImVec2(ChildWidth - 1.0f, 60.0f));

        //ImGui::EndChild();


        //ImGui::Text("CPU Timings:");
        //ImGui::Separator();

        //ImGui::Columns(2);

        //ImGui::Text("FPS:");
        //ImGui::NextColumn();

        //ImGui::Text("%d", gProfilerData.Fps);
        //ImGui::NextColumn();

        //ImGui::Text("FrameTime:");
        //ImGui::NextColumn();

        //const Float FtAvg = gProfilerData.FrameTime.GetAverage();
        //ImGui::Text("%.4f ms", FtAvg);
        //ImGui::PlotLines(
        //    "",
        //    gProfilerData.FrameTime.Samples.Data(),
        //    gProfilerData.FrameTime.SampleCount,
        //    gProfilerData.FrameTime.CurrentSample,
        //    nullptr,
        //    0.0f,
        //    ImGui_GetMaxLimit(FtAvg),
        //    ImVec2(0, 30.0f));

        //ImGui::Columns(1);

        //ImGui::Separator();

        //ImGui::Columns(2);

        //TStaticArray<Float, NUM_PROFILER_SAMPLES> Floats;
        //for (auto& Sample : gProfilerData.Samples)
        //{
        //    Memory::Memzero(Floats.Data(), Floats.SizeInBytes());

        //    Float Average = Sample.second.GetAverage();
        //    if (Average < MICROSECONDS)
        //    {
        //        for (Int32 n = 0; n < Sample.second.SampleCount; n++)
        //        {
        //            Floats[n] = Sample.second.Samples[n];
        //        }
        //    }
        //    else if (Average < MICROSECONDS)
        //    {
        //        for (Int32 n = 0; n < Sample.second.SampleCount; n++)
        //        {
        //            Floats[n] = Sample.second.Samples[n] * INV_MICROSECONDS;
        //        }
        //    }
        //    else if (Average < SECONDS)
        //    {
        //        for (Int32 n = 0; n < Sample.second.SampleCount; n++)
        //        {
        //            Floats[n] = Sample.second.Samples[n] * INV_MILLISECONDS;
        //        }
        //    }
        //    else
        //    {
        //        for (Int32 n = 0; n < Sample.second.SampleCount; n++)
        //        {
        //            Floats[n] = Sample.second.Samples[n] * INV_SECONDS;
        //        }
        //    }

        //    const Char* Name = Sample.first.c_str();
        //    ImGui_PrintTiming(Name, Average);

        //    ImGui::SameLine();

        //    ImGui_PrintTiming_SameLine("Min", Sample.second.Min);

        //    ImGui::SameLine();

        //    ImGui_PrintTiming_SameLine("Max", Sample.second.Max);

        //    if (Sample.second.SampleCount > 1)
        //    {
        //        const Float Avg = ImGui_ConvertNumber(Average);
        //        const Float Max = ImGui_GetMaxLimit(Avg);
        //        ImGui::PlotLines(
        //            "",
        //            Floats.Data(),
        //            Sample.second.SampleCount,
        //            Sample.second.CurrentSample,
        //            nullptr,
        //            0.0f,
        //            Max,
        //            ImVec2(0, 30.0f));
        //    }
        //    else
        //    {
        //        ImGui::NewLine();
        //    }

        //    ImGui::NextColumn();
        //}

        //ImGui::Columns(1);
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
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
    gDrawProfiler.SetBool(true);
}

void Profiler::Tick()
{
    Clock& Clock = gProfilerData.Clock;
    Clock.Tick();

    gProfilerData.CurrentFps++;
    if (Clock.GetTotalTime().AsSeconds() > 1.0f)
    {
        gProfilerData.Fps = gProfilerData.CurrentFps;
        gProfilerData.CurrentFps = 0;

        Clock.Reset();
    }

    if (gDrawFps.GetBool())
    {
        DebugUI::DrawUI(DrawFPS);
    }

    if (gDrawProfiler.GetBool())
    {
        const Double Delta = Clock.GetDeltaTime().AsMilliSeconds();
        gProfilerData.FrameTime.AddSample(Float(Delta));

        DebugUI::DrawUI(DrawProfiler);
    }
}

void Profiler::BeginTraceScope(const Char* Name)
{
    if (gProfilerData.EnableProfiler)
    {
        const std::string ScopeName = Name;

        auto Entry = gProfilerData.Samples.find(ScopeName);
        if (Entry == gProfilerData.Samples.end())
        {
            auto NewSample = gProfilerData.Samples.insert(std::make_pair(ScopeName, ProfileSample()));
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

        auto Entry = gProfilerData.Samples.find(ScopeName);
        if (Entry != gProfilerData.Samples.end())
        {
            Entry->second.End();
        }
        else
        {
            Assert(false);
        }
    }
}

void Profiler::SetGPUProfiler(GPUProfiler* Profiler)
{
    gProfilerData.GPUProfiler = MakeSharedRef<GPUProfiler>(Profiler);
}
