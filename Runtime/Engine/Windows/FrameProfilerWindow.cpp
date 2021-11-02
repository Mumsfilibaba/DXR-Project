#include "FrameProfilerWindow.h"

#include "Core/Application/Application.h"
#include "Core/Application/UI/UIUtilities.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Time/Timer.h"

#include <imgui.h>

TConsoleVariable<bool> GDrawFrameProfiler( false );
TConsoleVariable<bool> GDrawFps( false );

///////////////////////////////////////////////////////////////////////////////////////////////////

TSharedRef<CFrameProfilerWindow> CFrameProfilerWindow::Make()
{
    // Console Variables
    INIT_CONSOLE_VARIABLE( "r.DrawFps", &GDrawFps );
    INIT_CONSOLE_VARIABLE( "r.DrawFrameProfiler", &GDrawFrameProfiler );
    
    return dbg_new CFrameProfilerWindow();
}

void CFrameProfilerWindow::Tick()
{
    if ( GDrawFps.GetBool() )
    {
        DrawFPS();
    }

    if ( GDrawFrameProfiler.GetBool() )
    {
        DrawWindow();
    }
}

bool CFrameProfilerWindow::IsTickable()
{
    return GDrawFps.GetBool() || GDrawFrameProfiler.GetBool();
}

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
        CFrameProfiler::Get().GetCPUSamples( Samples );
        for ( auto& Sample : Samples )
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

        Samples.clear();

        ImGui::EndTable();
    }
}

void CFrameProfilerWindow::DrawWindow()
{
    // Draw DebugWindow with DebugStrings
    TSharedRef<CCoreWindow> MainViewport = CApplication::Get().GetMainViewport();

    const uint32 WindowWidth = MainViewport->GetWidth();
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

    bool TempDrawProfiler = GDrawFrameProfiler.GetBool();
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

        DrawCPUData( Width );

        ImGui::Separator();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::End();

    GDrawFrameProfiler.SetBool( TempDrawProfiler );
}
