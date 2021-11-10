#include "GameConsoleWindow.h"

#include "Core/Debug/Console/ConsoleManager.h"
#include "Interface/InterfaceApplication.h"
#include "Core/Templates/StringTraits.h"

#include <imgui.h>

TSharedRef<CGameConsoleWindow> CGameConsoleWindow::Make()
{
    TSharedRef<CGameConsoleWindow> NewWindow = dbg_new CGameConsoleWindow();
    
    NewWindow->InputHandler->HandleKeyEventDelegate.BindRaw( NewWindow.Get(), &CGameConsoleWindow::HandleKeyPressedEvent );
    CInterfaceApplication::Get().AddInputHandler( NewWindow->InputHandler );
    
    return NewWindow;
}

CGameConsoleWindow::CGameConsoleWindow()
    : IInterfaceWindow()
    , InputHandler( MakeShared<CConsoleInputHandler>() )
{
}

void CGameConsoleWindow::Tick()
{
    TSharedRef<CPlatformWindow> MainWindow = CInterfaceApplication::Get().GetMainViewport();

    const uint32 WindowWidth = MainWindow->GetWidth();
    const uint32 WindowHeight = MainWindow->GetHeight();
    // HACK: Push window up above titlebar to remove the rounded corners
    const ImVec2 Offset( 20.0f, -20.0f );

    const float Width = WindowWidth - (Offset.x * 2.0f);
    const float Height = 270.0f + Offset.y;

    ImGui::PushStyleColor( ImGuiCol_ResizeGrip, 0 );
    ImGui::PushStyleColor( ImGuiCol_ResizeGripHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ResizeGripActive, 0 );

    ImGui::SetNextWindowPos( ImVec2( Offset.x, Offset.y ), ImGuiCond_Always, ImVec2( 0.0f, 0.0f ) );
    ImGui::SetNextWindowSize( ImVec2( Width, Height ), ImGuiCond_Always );

    const ImGuiWindowFlags StyleFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 20.0f, 30.0f));

    ImGui::Begin( "Console Window", nullptr, StyleFlags );
    {
        ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, ImVec4( 0.3f, 0.3f, 0.3f, 0.6f ) );

        const ImVec2 ParentSize = ImGui::GetWindowSize();
        const float TextWindowWidth = Width * 0.985f;
        const float TextWindowHeight = ParentSize.y - 64.0f;

        const ImGuiWindowFlags PopupFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::BeginChild( "##ChildWindow", ImVec2( TextWindowWidth, TextWindowHeight ), false, PopupFlags );
        if ( !Candidates.IsEmpty() )
        {
            bool IsActiveIndex = false;

            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 2 ) );
            ImGui::PushAllowKeyboardFocus( false );

            ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0.4f, 0.4f, 0.4f, 1.0f ) );
            ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0.2f, 0.2f, 0.2f, 1.0f ) );

            const float Padding = 8.0f;
            float VariableNameWidth  = 30.0f;
            float VariableValueWidth = 20.0f;

            // First find the maximum length of each column for the selectable
            Candidates.Foreach([&]( const TPair<IConsoleObject*, CString>& Candidate )
            {
                VariableNameWidth = NMath::Max( VariableNameWidth, ImGui::CalcTextSize( Candidate.Second.CStr() ).x );

                IConsoleVariable* Variable = Candidate.First->AsVariable();
                if ( Variable )
                {
                    CString Value = Variable->GetString();
                    VariableValueWidth = NMath::Max( VariableValueWidth, ImGui::CalcTextSize( Value.CStr() ).x );
                }
            });

            VariableNameWidth  += Padding;
            VariableValueWidth += Padding;

            // Draw UI
            for ( int32 i = 0; i < Candidates.Size(); i++ )
            {
                const TPair<IConsoleObject*, CString>& Candidate = Candidates[i];
                IsActiveIndex = (CandidatesIndex == i);

                // VariableName
                ImGui::PushID( i );
                if ( ImGui::Selectable( Candidate.Second.CStr(), &IsActiveIndex ) )
                {
                    CStringTraits::Copy( TextBuffer.Data(), Candidate.Second.CStr() );
                    PopupSelectedText = Candidate.Second;

                    Candidates.Clear();
                    CandidatesIndex = -1;

                    UpdateCursorPosition = true;

                    ImGui::PopID();
                    break;
                }

                ImGui::SameLine( VariableNameWidth );

                ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ) );

                const char* PostFix = "";
                
                // Value
                IConsoleVariable* Variable = Candidate.First->AsVariable();
                if ( Variable )
                {
                    CString Value = Variable->GetString();
                    ImGui::Text( "%s", Value.CStr() );

                    if ( Variable->IsBool() )
                    {
                        PostFix = "Boolean";
                    }
                    else if ( Variable->IsInt() )
                    {
                        PostFix = "Integer";
                    }
                    else if ( Variable->IsFloat() )
                    {
                        PostFix = "Float";
                    }
                    else if ( Variable->IsString() )
                    {
                        PostFix = "String";
                    }
                }
                else if ( Candidate.First->AsCommand() )
                {
                    PostFix = "Command";
                }
                
                // Offset from the start is name + value 
                ImGui::SameLine( VariableNameWidth + VariableValueWidth );
                
                // PostFix
                ImGui::Text( "[%s]", PostFix );

                ImGui::PopStyleColor();

                ImGui::PopID();

                if ( IsActiveIndex && CandidateSelectionChanged )
                {
                    ImGui::SetScrollHere();
                    PopupSelectedText = Candidate.Second;
                    CandidateSelectionChanged = false;
                }
            }

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            ImGui::PopAllowKeyboardFocus();
            ImGui::PopStyleVar();
        }
        else
        {
            const TArray<TPair<CString, EConsoleSeverity>>& ConsoleMessages = CConsoleManager::Get().GetMessages();
            for ( const TPair<CString, EConsoleSeverity>& Text : ConsoleMessages )
            {
                ImVec4 Color;
                if ( Text.Second == EConsoleSeverity::Info )
                {
                    Color = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
                }
                else if ( Text.Second == EConsoleSeverity::Warning )
                {
                    Color = ImVec4( 1.0f, 1.0f, 0.0f, 1.0f );
                }
                else if ( Text.Second == EConsoleSeverity::Error )
                {
                    Color = ImVec4( 1.0f, 0.0f, 0.0f, 1.0f );
                }

                ImGui::TextColored( Color, "%s", Text.First.CStr() );
            }

            if ( ScrollDown )
            {
                ImGui::SetScrollHereY();
                ScrollDown = false;
            }
        }

        ImGui::EndChild();

        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.1f, 0.1f, 0.1f, 0.5f ) );

        // Draw the Input Sign for the text input 
        {
            ImVec2 CursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos( ImVec2( CursorPos.x, CursorPos.y + 2.0f) );

            ImGui::Text( ">" );

            ImGui::SameLine();
            
            CursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos( ImVec2( CursorPos.x, CursorPos.y - 2.0f ) );
        }

        // Text Input
        ImGui::PushItemWidth( TextWindowWidth - 25.0f );

        const ImGuiInputTextFlags InputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackCompletion |
            ImGuiInputTextFlags_CallbackHistory |
            ImGuiInputTextFlags_CallbackAlways |
            ImGuiInputTextFlags_CallbackEdit;

        // Prepare callback for ImGui
        auto Callback = []( ImGuiInputTextCallbackData* Data )->int32
        {
            CGameConsoleWindow* This = reinterpret_cast<CGameConsoleWindow*>(Data->UserData);
            return This->TextCallback( Data );
        };

        const bool Result = ImGui::InputText( "###Input", TextBuffer.Data(), TextBuffer.Size(), InputFlags, Callback, reinterpret_cast<void*>(this) );
        if ( Result && TextBuffer[0] != 0 )
        {
            if ( CandidatesIndex != -1 )
            {
                CStringTraits::Copy( TextBuffer.Data(), PopupSelectedText.CStr() );

                Candidates.Clear();
                CandidatesIndex = -1;

                UpdateCursorPosition = true;
            }
            else
            {
                const CString Text = CString( TextBuffer.Data() );
                CConsoleManager::Get().Execute( Text );

                TextBuffer[0] = 0;
                ScrollDown = true;

                ImGui::SetItemDefaultFocus();
                ImGui::SetKeyboardFocusHere( -1 );
            }
        }

        if ( ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked( 0 ) )
        {
            ImGui::SetKeyboardFocusHere( -1 );
        }

        ImGui::PopItemWidth();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

    }
    ImGui::End();

    ImGui::PopStyleVar();
}

bool CGameConsoleWindow::IsTickable()
{
    return IsActive;
}

int32 CGameConsoleWindow::TextCallback( ImGuiInputTextCallbackData* Data )
{
    if ( UpdateCursorPosition )
    {
        Data->CursorPos = int32( PopupSelectedText.Length() );
        PopupSelectedText.Clear();

        UpdateCursorPosition = false;
    }

    switch ( Data->EventFlag )
    {
        case ImGuiInputTextFlags_CallbackEdit:
        {
            const char* WordEnd = Data->Buf + Data->CursorPos;
            const char* WordStart = WordEnd;
            while ( WordStart > Data->Buf )
            {
                const char c = WordStart[-1];
                if ( c == ' ' || c == '\t' || c == ',' || c == ';' )
                {
                    break;
                }

                WordStart--;
            }

            Candidates.Clear();
            CandidateSelectionChanged = true;
            CandidatesIndex = -1;

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if ( WordLength <= 0 )
            {
                break;
            }

            const CStringView CandidateName( WordStart, WordLength );
            CConsoleManager::Get().FindCandidates( CandidateName, Candidates );
            break;
        }
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            const char* WordEnd = Data->Buf + Data->CursorPos;
            const char* WordStart = WordEnd;
            if ( Data->BufTextLen > 0 )
            {
                while ( WordStart > Data->Buf )
                {
                    const char c = WordStart[-1];
                    if ( c == ' ' || c == '\t' || c == ',' || c == ';' )
                    {
                        break;
                    }

                    WordStart--;
                }
            }

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if ( WordLength > 0 )
            {
                if ( Candidates.Size() == 1 )
                {
                    const int32 Pos = static_cast<int32>(WordStart - Data->Buf);
                    const int32 Count = WordLength;
                    Data->DeleteChars( Pos, Count );
                    Data->InsertChars( Data->CursorPos, Candidates[0].Second.CStr() );

                    CandidatesIndex = -1;
                    CandidateSelectionChanged = true;
                    Candidates.Clear();
                }
                else if ( !Candidates.IsEmpty() && CandidatesIndex != -1 )
                {
                    const int32 Pos = static_cast<int32>(WordStart - Data->Buf);
                    const int32 Count = WordLength;
                    Data->DeleteChars( Pos, Count );
                    Data->InsertChars( Data->CursorPos, PopupSelectedText.CStr() );

                    PopupSelectedText = "";

                    Candidates.Clear();
                    CandidatesIndex = -1;
                    CandidateSelectionChanged = true;
                }
            }

            break;
        }
        case ImGuiInputTextFlags_CallbackHistory:
        {
            if ( Candidates.IsEmpty() )
            {
                const TArray<CString>& History = CConsoleManager::Get().GetHistory();
                if ( History.IsEmpty() )
                {
                    HistoryIndex = -1;
                }

                const int32 PrevHistoryIndex = HistoryIndex;
                if ( Data->EventKey == ImGuiKey_UpArrow )
                {
                    if ( HistoryIndex == -1 )
                    {
                        HistoryIndex = History.Size() - 1;
                    }
                    else if ( HistoryIndex > 0 )
                    {
                        HistoryIndex--;
                    }
                }
                else if ( Data->EventKey == ImGuiKey_DownArrow )
                {
                    if ( HistoryIndex != -1 )
                    {
                        HistoryIndex++;
                        if ( HistoryIndex >= static_cast<int32>(History.Size()) )
                        {
                            HistoryIndex = -1;
                        }
                    }
                }

                if ( PrevHistoryIndex != HistoryIndex )
                {
                    const char* HistoryStr = (HistoryIndex >= 0) ? History[HistoryIndex].CStr() : "";
                    Data->DeleteChars( 0, Data->BufTextLen );
                    Data->InsertChars( 0, HistoryStr );
                }
            }
            else
            {
                if ( Data->EventKey == ImGuiKey_UpArrow )
                {
                    CandidateSelectionChanged = true;
                    if ( CandidatesIndex <= 0 )
                    {
                        CandidatesIndex = Candidates.Size() - 1;
                    }
                    else
                    {
                        CandidatesIndex--;
                    }
                }
                else if ( Data->EventKey == ImGuiKey_DownArrow )
                {
                    CandidateSelectionChanged = true;
                    if ( CandidatesIndex >= int32( Candidates.Size() ) - 1 )
                    {
                        CandidatesIndex = 0;
                    }
                    else
                    {
                        CandidatesIndex++;
                    }
                }
            }

            break;
        }
    }

    return 0;
}

void CGameConsoleWindow::HandleKeyPressedEvent( const SKeyEvent& Event )
{
    Assert( InputHandler.IsValid() );

    InputHandler->ConsoleToggled = false;

    if ( Event.IsDown )
    {
        if ( !Event.IsRepeat && Event.KeyCode == EKey::Key_GraveAccent )
        {
            IsActive = !IsActive;
            InputHandler->ConsoleToggled = true;
        }
    }
}