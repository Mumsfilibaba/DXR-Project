#include "Profiler.h"
#include "Console/Console.h"

#include "Rendering/DebugUI.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/GPUProfiler.h"

#include "Core/Engine/Engine.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Platform/Mutex.h"

constexpr float MICROSECONDS = 1000.0f;
constexpr float MILLISECONDS = 1000.0f * 1000.0f;
constexpr float SECONDS = 1000.0f * 1000.0f * 1000.0f;
constexpr float INV_MICROSECONDS = 1.0f / MICROSECONDS;
constexpr float INV_MILLISECONDS = 1.0f / MILLISECONDS;
constexpr float INV_SECONDS = 1.0f / SECONDS;

constexpr float MAX_FRAMETIME_MS = 1000.0f / 30.0f;

TConsoleVariable<bool> GDrawProfiler( false );
TConsoleVariable<bool> GDrawFps( false );

struct ProfileSample
{
    FORCEINLINE void Begin()
    {
        Clock.Tick();
    }

    FORCEINLINE void End()
    {
        Clock.Tick();

        float Delta = (float)Clock.GetDeltaTime().AsNanoSeconds();
        AddSample( Delta );

        TotalCalls++;
    }

    FORCEINLINE void AddSample( float NewSample )
    {
        Samples[CurrentSample] = NewSample;
        Min = NMath::Min( NewSample, Min );
        Max = NMath::Max( NewSample, Max );

        CurrentSample++;
        SampleCount = NMath::Min<int32>( Samples.Size(), SampleCount + 1 );

        if ( CurrentSample >= int32( Samples.Size() ) )
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE float GetAverage() const
    {
        if ( SampleCount < 1 )
        {
            return 0.0f;
        }

        float Average = 0.0f;
        for ( int32 n = 0; n < SampleCount; n++ )
        {
            Average += Samples[n];
        }

        return Average / float( SampleCount );
    }

    FORCEINLINE void Reset()
    {
        Samples.Fill( 0.0f );
        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;
        Max = -FLT_MAX;
        Min = FLT_MAX;
        Clock.Reset();
    }

    TStaticArray<float, NUM_PROFILER_SAMPLES> Samples;
    Timer Clock;
    float Max = -FLT_MAX;
    float Min = FLT_MAX;
    int32 SampleCount = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls = 0;
};

struct GPUProfileSample
{
    FORCEINLINE void AddSample( float NewSample )
    {
        Samples[CurrentSample] = NewSample;
        Min = NMath::Min( NewSample, Min );
        Max = NMath::Max( NewSample, Max );

        CurrentSample++;
        SampleCount = NMath::Min<int32>( Samples.Size(), SampleCount + 1 );

        if ( CurrentSample >= int32( Samples.Size() ) )
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE float GetAverage() const
    {
        if ( SampleCount < 1 )
        {
            return 0.0f;
        }

        float Average = 0.0f;
        for ( int32 n = 0; n < SampleCount; n++ )
        {
            Average += Samples[n];
        }

        return Average / float( SampleCount );
    }

    FORCEINLINE void Reset()
    {
        Samples.Fill( 0.0f );
        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;
        Max = -FLT_MAX;
        Min = FLT_MAX;
    }

    TStaticArray<float, NUM_PROFILER_SAMPLES> Samples;
    float  Max = -FLT_MAX;
    float  Min = FLT_MAX;
    int32  SampleCount = 0;
    int32  CurrentSample = 0;
    int32  TotalCalls = 0;
    uint32 TimeQueryIndex = 0;
};

struct ProfilerData
{
    TSharedRef<GPUProfiler> GPUProfiler;

    uint32 CurrentTimeQueryIndex = 0;

    ProfileSample    CPUFrameTime;
    GPUProfileSample GPUFrameTime;

    Timer Clock;

    int32 CurrentFps = 0;
    int32 Fps = 0;

    bool EnableProfiler = true;

    std::unordered_map<std::string, ProfileSample> CPUSamples;
    Mutex CPUSamplesMutex;

    std::unordered_map<std::string, GPUProfileSample> GPUSamples;
    Mutex GPUSamplesMutex;
};

static ProfilerData GProfilerData;

static void ImGui_PrintTime( float Num )
{
    if ( Num == FLT_MAX || Num == -FLT_MAX )
    {
        ImGui::Text( "0.0 s" );
    }
    else if ( Num < MICROSECONDS )
    {
        ImGui::Text( "%.4f ns", Num );
    }
    else if ( Num < MICROSECONDS )
    {
        const float Time = Num * INV_MICROSECONDS;
        ImGui::Text( "%.4f qs", Time );
    }
    else if ( Num < SECONDS )
    {
        const float Time = Num * INV_MILLISECONDS;
        ImGui::Text( "%.4f ms", Time );
    }
    else
    {
        const float Time = Num * INV_SECONDS;
        ImGui::Text( "%.4f s", Time );
    }
}

static void ImGui_PrintTiming( const char* Text, float Num )
{
    ImGui::Text( "%s: ", Text );

    ImGui::NextColumn();

    ImGui_PrintTime( Num );
}

static void ImGui_PrintTiming_SameLine( const char* Text, float Num )
{
    ImGui::Text( "%s: ", Text );
    ImGui::SameLine();
    ImGui_PrintTime( Num );
}

static float ImGui_ConvertNumber( float Num )
{
    if ( Num < MICROSECONDS )
    {
        return Num;
    }
    else if ( Num < MICROSECONDS )
    {
        return Num * INV_MICROSECONDS;
    }
    else if ( Num < SECONDS )
    {
        return Num * INV_MILLISECONDS;
    }
    else
    {
        return Num * INV_SECONDS;
    }
}

static float ImGui_GetMaxLimit( float Num )
{
    if ( Num < 0.01f )
    {
        return 0.01f;
    }
    else if ( Num < 0.1f )
    {
        return 0.1f;
    }
    else if ( Num < 1.0f )
    {
        return 1.0f;
    }
    else if ( Num < 3.0f )
    {
        return 3.0f;
    }
    else if ( Num < 17.0f )
    {
        return 17.0f;
    }
    else if ( Num < 34.0f )
    {
        return 34.0f;
    }
    else if ( Num < 100.0f )
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
    const uint32 WindowWidth = GEngine.MainWindow->GetWidth();

    ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2( 5.0f, 5.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 2.0f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.0f, 1.0f, 0.2f, 1.0f ) );

    ImGui::SetNextWindowPos( ImVec2( float( WindowWidth ), 0.0f ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin( "FPS Window", nullptr, Flags );

    const std::string FpsStr = std::to_string( GProfilerData.Fps );
    ImGui::Text( "%s", FpsStr.c_str() );

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

static void DrawCPUProfileData( float Width )
{
    const ImGuiTableFlags TableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg;

    if ( ImGui::BeginTable( "Frame Statistics", 1, TableFlags ) )
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );

        float Avg = GProfilerData.CPUFrameTime.GetAverage();
        float Min = GProfilerData.CPUFrameTime.Min;
        if ( Min == FLT_MAX )
        {
            Min = 0.0f;
        }

        float Max = GProfilerData.CPUFrameTime.Max;
        if ( Max == -FLT_MAX )
        {
            Max = 0.0f;
        }

        ImGui::Text( "FrameTime:" );
        ImGui::SameLine();
        ImGui::Text( "Avg: %.4f ms", Avg );
        ImGui::SameLine();
        ImGui::Text( "Min: %.4f ms", Min );
        ImGui::SameLine();
        ImGui::Text( "Max: %.4f ms", Max );

        ImGui::NewLine();

        ImGui::PlotHistogram(
            "",
            GProfilerData.CPUFrameTime.Samples.Data(),
            GProfilerData.CPUFrameTime.SampleCount,
            GProfilerData.CPUFrameTime.CurrentSample,
            nullptr,
            0.0f,
            ImGui_GetMaxLimit( Avg ),
            ImVec2( Width * 0.9825f, 80.0f ) );

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

    if ( ImGui::BeginTable( "Functions", 5, TableFlags ) )
    {
        ImGui::TableSetupColumn( "Trace Name" );
        ImGui::TableSetupColumn( "Total Calls" );
        ImGui::TableSetupColumn( "Avg" );
        ImGui::TableSetupColumn( "Min" );
        ImGui::TableSetupColumn( "Max" );
        ImGui::TableHeadersRow();

        for ( auto& Sample : GProfilerData.CPUSamples )
        {
            ImGui::TableNextRow();

            float Avg = Sample.second.GetAverage();
            float Min = Sample.second.Min;
            float Max = Sample.second.Max;
            int32 Calls = Sample.second.TotalCalls;

            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%s", Sample.first.c_str() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui::Text( "%d", Calls );
            ImGui::TableSetColumnIndex( 2 );
            ImGui_PrintTime( Avg );
            ImGui::TableSetColumnIndex( 3 );
            ImGui_PrintTime( Min );
            ImGui::TableSetColumnIndex( 4 );
            ImGui_PrintTime( Max );
        }

        ImGui::EndTable();
    }
}

static void DrawGPUProfileData( float Width )
{
    const ImGuiTableFlags TableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg;

    if ( ImGui::BeginTable( "Frame Statistics", 1, TableFlags ) )
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );

        float Avg = GProfilerData.GPUFrameTime.GetAverage();
        float Min = GProfilerData.GPUFrameTime.Min;
        if ( Min == FLT_MAX )
        {
            Min = 0.0f;
        }

        float Max = GProfilerData.GPUFrameTime.Max;
        if ( Max == -FLT_MAX )
        {
            Max = 0.0f;
        }

        ImGui::Text( "FrameTime:" );
        ImGui::SameLine();
        ImGui::Text( "Avg: %.4f ms", Avg );
        ImGui::SameLine();
        ImGui::Text( "Min: %.4f ms", Min );
        ImGui::SameLine();
        ImGui::Text( "Max: %.4f ms", Max );

        ImGui::NewLine();

        ImGui::PlotHistogram(
            "",
            GProfilerData.GPUFrameTime.Samples.Data(),
            GProfilerData.GPUFrameTime.SampleCount,
            GProfilerData.GPUFrameTime.CurrentSample,
            nullptr,
            0.0f,
            ImGui_GetMaxLimit( Avg ),
            ImVec2( Width * 0.9825f, 80.0f ) );

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

    if ( ImGui::BeginTable( "Functions", 4, TableFlags ) )
    {
        ImGui::TableSetupColumn( "Trace Name" );
        ImGui::TableSetupColumn( "Avg" );
        ImGui::TableSetupColumn( "Min" );
        ImGui::TableSetupColumn( "Max" );
        ImGui::TableHeadersRow();

        for ( auto& Sample : GProfilerData.GPUSamples )
        {
            ImGui::TableNextRow();

            float Avg = Sample.second.GetAverage();
            float Min = Sample.second.Min;
            float Max = Sample.second.Max;

            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%s", Sample.first.c_str() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui_PrintTime( Avg );
            ImGui::TableSetColumnIndex( 2 );
            ImGui_PrintTime( Min );
            ImGui::TableSetColumnIndex( 3 );
            ImGui_PrintTime( Max );
        }

        ImGui::EndTable();
    }
}

static void DrawProfiler()
{
    // Draw DebugWindow with DebugStrings
    const uint32 WindowWidth = GEngine.MainWindow->GetWidth();
    const uint32 WindowHeight = GEngine.MainWindow->GetHeight();
    const float Width = NMath::Max( WindowWidth * 0.6f, 400.0f );
    const float Height = WindowHeight * 0.75f;

    ImGui::PushStyleColor( ImGuiCol_ResizeGrip, 0 );
    ImGui::PushStyleColor( ImGuiCol_ResizeGripHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ResizeGripActive, 0 );

    ImGui::SetNextWindowPos( ImVec2( float( WindowWidth ) * 0.5f, float( WindowHeight ) * 0.175f ), ImGuiCond_Appearing, ImVec2( 0.5f, 0.0f ) );
    ImGui::SetNextWindowSize( ImVec2( Width, Height ), ImGuiCond_Appearing );

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    bool TempDrawProfiler = GDrawProfiler.GetBool();
    if ( ImGui::Begin( "Profiler", &TempDrawProfiler, Flags ) )
    {
        if ( ImGui::Button( "Start Profile" ) )
        {
            Profiler::Enable();
        }

        ImGui::SameLine();

        if ( ImGui::Button( "Stop Profile" ) )
        {
            Profiler::Disable();
        }

        ImGui::SameLine();

        if ( ImGui::Button( "Reset" ) )
        {
            Profiler::Reset();
        }

        ImGuiTabBarFlags TabBarFlags = ImGuiTabBarFlags_None;
        if ( ImGui::BeginTabBar( "ProfilerTabs", TabBarFlags ) )
        {
            if ( ImGui::BeginTabItem( "CPU" ) )
            {
                DrawCPUProfileData( Width );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( "GPU" ) )
            {
                DrawGPUProfileData( Width );
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

    GDrawProfiler.SetBool( TempDrawProfiler );
}

void Profiler::Init()
{
    INIT_CONSOLE_VARIABLE( "r.DrawFps", &GDrawFps );
    INIT_CONSOLE_VARIABLE( "r.DrawProfiler", &GDrawProfiler );
}

void Profiler::Tick()
{
    Timer& Clock = GProfilerData.Clock;
    Clock.Tick();

    GProfilerData.CurrentFps++;
    if ( Clock.GetTotalTime().AsSeconds() > 1.0f )
    {
        GProfilerData.Fps = GProfilerData.CurrentFps;
        GProfilerData.CurrentFps = 0;

        Clock.Reset();
    }

    if ( GDrawFps.GetBool() )
    {
        DebugUI::DrawUI( DrawFPS );
    }

    if ( GDrawProfiler.GetBool() )
    {
        if ( GProfilerData.EnableProfiler )
        {
            const double Delta = Clock.GetDeltaTime().AsMilliSeconds();
            GProfilerData.CPUFrameTime.AddSample( float( Delta ) );

            if ( GProfilerData.GPUProfiler )
            {
                TimeQuery Query;
                GProfilerData.GPUProfiler->GetTimeQuery( Query, GProfilerData.GPUFrameTime.TimeQueryIndex );

                double Frequency = (double)GProfilerData.GPUProfiler->GetFrequency();
                double DeltaTime = (double)(Query.End - Query.Begin);
                double Duration = (DeltaTime / Frequency) * 1000.0;
                GProfilerData.GPUFrameTime.AddSample( (float)Duration );
            }
        }

        DebugUI::DrawUI( DrawProfiler );
    }
}

void Profiler::Enable()
{
    GProfilerData.EnableProfiler = true;
}

void Profiler::Disable()
{
    GProfilerData.EnableProfiler = false;
}

void Profiler::Reset()
{
    GProfilerData.CPUFrameTime.Reset();
    GProfilerData.GPUFrameTime.Reset();

    {
        TScopedLock<Mutex> Lock( GProfilerData.CPUSamplesMutex );
        for ( auto& Sample : GProfilerData.CPUSamples )
        {
            Sample.second.Reset();
        }
    }

    {
        TScopedLock<Mutex> Lock( GProfilerData.GPUSamplesMutex );
        for ( auto& Sample : GProfilerData.GPUSamples )
        {
            Sample.second.Reset();
        }
    }
}

void Profiler::BeginTraceScope( const char* Name )
{
    if ( GProfilerData.EnableProfiler )
    {
        const std::string ScopeName = Name;

        TScopedLock<Mutex> Lock( GProfilerData.CPUSamplesMutex );

        auto Entry = GProfilerData.CPUSamples.find( ScopeName );
        if ( Entry == GProfilerData.CPUSamples.end() )
        {
            auto NewSample = GProfilerData.CPUSamples.insert( std::make_pair( ScopeName, ProfileSample() ) );
            NewSample.first->second.Begin();
        }
        else
        {
            Entry->second.Begin();
        }
    }
}

void Profiler::EndTraceScope( const char* Name )
{
    if ( GProfilerData.EnableProfiler )
    {
        const std::string ScopeName = Name;

        TScopedLock<Mutex> Lock( GProfilerData.CPUSamplesMutex );

        auto Entry = GProfilerData.CPUSamples.find( ScopeName );
        if ( Entry != GProfilerData.CPUSamples.end() )
        {
            Entry->second.End();
        }
        else
        {
            Assert( false );
        }
    }
}

void Profiler::BeginGPUFrame( CommandList& CmdList )
{
    if ( GProfilerData.GPUProfiler && GProfilerData.EnableProfiler )
    {
        CmdList.BeginTimeStamp( GProfilerData.GPUProfiler.Get(), GProfilerData.GPUFrameTime.TimeQueryIndex );
    }
}

void Profiler::BeginGPUTrace( CommandList& CmdList, const char* Name )
{
    if ( GProfilerData.GPUProfiler && GProfilerData.EnableProfiler )
    {
        const std::string ScopeName = Name;

        int32 TimeQueryIndex = -1;

        {
            TScopedLock<Mutex> Lock( GProfilerData.GPUSamplesMutex );

            auto Entry = GProfilerData.GPUSamples.find( ScopeName );
            if ( Entry == GProfilerData.GPUSamples.end() )
            {
                auto NewSample = GProfilerData.GPUSamples.insert( std::make_pair( ScopeName, GPUProfileSample() ) );
                NewSample.first->second.TimeQueryIndex = ++GProfilerData.CurrentTimeQueryIndex;
                TimeQueryIndex = NewSample.first->second.TimeQueryIndex;
            }
            else
            {
                TimeQueryIndex = Entry->second.TimeQueryIndex;
            }
        }

        if ( TimeQueryIndex >= 0 )
        {
            CmdList.BeginTimeStamp( GProfilerData.GPUProfiler.Get(), TimeQueryIndex );
        }
    }
}

void Profiler::EndGPUTrace( CommandList& CmdList, const char* Name )
{
    if ( GProfilerData.GPUProfiler && GProfilerData.EnableProfiler )
    {
        const std::string ScopeName = Name;

        int32 TimeQueryIndex = -1;

        TScopedLock<Mutex> Lock( GProfilerData.GPUSamplesMutex );

        auto Entry = GProfilerData.GPUSamples.find( ScopeName );
        if ( Entry != GProfilerData.GPUSamples.end() )
        {
            TimeQueryIndex = Entry->second.TimeQueryIndex;
            CmdList.EndTimeStamp( GProfilerData.GPUProfiler.Get(), TimeQueryIndex );

            if ( TimeQueryIndex >= 0 )
            {
                TimeQuery Query;
                GProfilerData.GPUProfiler->GetTimeQuery( Query, TimeQueryIndex );

                double Frequency = (double)GProfilerData.GPUProfiler->GetFrequency();
                double Duration = (double)((Query.End - Query.Begin) / Frequency) * SECONDS;
                Entry->second.AddSample( (float)Duration );
            }
        }
    }
}

void Profiler::SetGPUProfiler( GPUProfiler* Profiler )
{
    GProfilerData.GPUProfiler = MakeSharedRef<GPUProfiler>( Profiler );
}

void Profiler::EndGPUFrame( CommandList& CmdList )
{
    if ( GProfilerData.GPUProfiler && GProfilerData.EnableProfiler )
    {
        CmdList.EndTimeStamp( GProfilerData.GPUProfiler.Get(), GProfilerData.GPUFrameTime.TimeQueryIndex );
    }
}