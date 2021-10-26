#include "FrameProfilerWindow.h"

#include "Core/Application/Application.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Time/Timer.h"

#include <imgui.h>

TConsoleVariable<bool> GDrawProfiler( false );
TConsoleVariable<bool> GDrawFps( false );

constexpr float MAX_FRAMETIME_MS = 1000.0f / 30.0f;

/* Helpers */

static void ImGui_PrintTime( float Nanoseconds )
{
    if ( Nanoseconds == FLT_MAX || Nanoseconds == -FLT_MAX )
    {
        ImGui::Text( "0.0 s" );
    }
    else if ( Nanoseconds < NTime::FromMicroseconds<float>( 1.0f ) )
    {
        ImGui::Text( "%.4f ns", Nanoseconds );
    }
    else if ( Nanoseconds < NTime::FromMilliseconds<float>( 1.0f ) )
    {
        ImGui::Text( "%.4f qs", NTime::ToMicroseconds<float>( Nanoseconds ) );
    }
    else if ( Nanoseconds < NTime::FromSeconds<float>( 1.0f ) )
    {
        ImGui::Text( "%.4f ms", NTime::ToMilliseconds<float>( Nanoseconds ) );
    }
    else
    {
        ImGui::Text( "%.4f s", NTime::ToSeconds<float>( Nanoseconds ) );
    }
}

#if 0
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

#endif

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

/* Implementation */

void CFrameProfilerWindow::DrawFPS()
{
    const uint32 WindowWidth = CApplication::Get().GetMainViewport()->GetWidth();

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

    const CString FpsString = ToString( CFrameProfiler::Get().GetFramesPerSecond() );
    ImGui::Text( "%s", FpsString.CStr() );

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void CFrameProfilerWindow::DrawCPUData( float Width )
{
    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if ( ImGui::BeginTable( "Frame Statistics", 1, TableFlags ) )
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );

        const SProfileSample& CPUFrameTime = CFrameProfiler::Get().GetCPUFrameTime();

        float Avg = CPUFrameTime.GetAverage();
        float Min = CPUFrameTime.Min;
        if ( Min == FLT_MAX )
        {
            Min = 0.0f;
        }

        float Max = CPUFrameTime.Max;
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
            CPUFrameTime.Samples.Data(),
            CPUFrameTime.SampleCount,
            CPUFrameTime.CurrentSample,
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

        // Retrieve a copy of the CPU samples
        CFrameProfiler::Get().GetCPUSamples( CPUSamples );
        for ( auto& Sample : CPUSamples )
        {
            ImGui::TableNextRow();

            float Avg = Sample.second.GetAverage();
            float Min = Sample.second.Min;
            float Max = Sample.second.Max;
            int32 Calls = Sample.second.TotalCalls;

            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%s", Sample.first.CStr() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui::Text( "%d", Calls );
            ImGui::TableSetColumnIndex( 2 );
            ImGui_PrintTime( Avg );
            ImGui::TableSetColumnIndex( 3 );
            ImGui_PrintTime( Min );
            ImGui::TableSetColumnIndex( 4 );
            ImGui_PrintTime( Max );
        }

        CPUSamples.clear();

        ImGui::EndTable();
    }
}

void CFrameProfilerWindow::DrawGPUData( float Width )
{
    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    if ( ImGui::BeginTable( "Frame Statistics", 1, TableFlags ) )
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );

        const SGPUProfileSample& GPUFrameTime = CFrameProfiler::Get().GetGPUFrameTime();

        float Avg = GPUFrameTime.GetAverage();
        float Min = GPUFrameTime.Min;
        if ( Min == FLT_MAX )
        {
            Min = 0.0f;
        }

        float Max = GPUFrameTime.Max;
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
            GPUFrameTime.Samples.Data(),
            GPUFrameTime.SampleCount,
            GPUFrameTime.CurrentSample,
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

        CFrameProfiler::Get().GetGPUSamples( GPUSamples );
        for ( auto& Sample : GPUSamples )
        {
            ImGui::TableNextRow();

            float Avg = Sample.second.GetAverage();
            float Min = Sample.second.Min;
            float Max = Sample.second.Max;

            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%s", Sample.first.CStr() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui_PrintTime( Avg );
            ImGui::TableSetColumnIndex( 2 );
            ImGui_PrintTime( Min );
            ImGui::TableSetColumnIndex( 3 );
            ImGui_PrintTime( Max );
        }

        GPUSamples.clear();

        ImGui::EndTable();
    }
}

void CFrameProfilerWindow::DrawWindow()
{
    // Draw DebugWindow with DebugStrings
    TSharedRef<CCoreWindow> MainViewport = CApplication::Get().GetMainViewport();

    const uint32 WindowWidth  = MainViewport->GetWidth();
    const uint32 WindowHeight = MainViewport->GetHeight();

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
            CFrameProfiler::Get().Enable();
        }

        ImGui::SameLine();

        if ( ImGui::Button( "Stop Profile" ) )
        {
            CFrameProfiler::Get().Disable();
        }

        ImGui::SameLine();

        if ( ImGui::Button( "Reset" ) )
        {
            CFrameProfiler::Get().Reset();
        }

        ImGuiTabBarFlags TabBarFlags = ImGuiTabBarFlags_None;
        if ( ImGui::BeginTabBar( "ProfilerTabs", TabBarFlags ) )
        {
            if ( ImGui::BeginTabItem( "CPU" ) )
            {
                DrawCPUData( Width );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( "GPU" ) )
            {
                DrawGPUData( Width );
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

/* Implementation */

TSharedRef<CFrameProfilerWindow> CFrameProfilerWindow::Make()
{
    return dbg_new CFrameProfilerWindow();
}

void CFrameProfilerWindow::InitContext( UIContextHandle ContextHandle )
{
    // Context
    INIT_CONTEXT( ContextHandle );

    // Console Variables
    INIT_CONSOLE_VARIABLE( "r.DrawFps", &GDrawFps );
    INIT_CONSOLE_VARIABLE( "r.DrawProfiler", &GDrawProfiler );
}

void CFrameProfilerWindow::Tick()
{
    if ( GDrawFps.GetBool() )
    {
        DrawFPS();
    }

    if ( GDrawProfiler.GetBool() )
    {
        DrawWindow();
    }
}

bool CFrameProfilerWindow::IsTickable()
{
    return GDrawFps.GetBool() || GDrawProfiler.GetBool();
}
