#include "ConsoleManager.h"

#include "Rendering/UIRenderer.h"

#include "Core/Engine/EngineLoop.h"
#include "Core/Engine/Engine.h"

#include "Core/Application/Application.h"

#include <regex>

CConsoleManager GConsole;

CConsoleCommand GClearHistory;

TConsoleVariable<CString> GEcho;

void CConsoleManager::Init()
{
    GClearHistory.GetExecutedDelgate().AddRaw( this, &CConsoleManager::ClearHistory );
    INIT_CONSOLE_COMMAND( "ClearHistory", &GClearHistory );

    GEcho.GetChangedDelegate().AddLambda([this]( IConsoleVariable* InVariable ) -> void
    {
        if ( InVariable->IsString() )
        {
            this->PrintMessage( InVariable->GetString() );
        }
    });

    INIT_CONSOLE_VARIABLE( "Echo", &GEcho );

    InputHandler.HandleKeyEventDelegate.BindRaw( this, &CConsoleManager::OnKeyPressedEvent );
    CApplication::Get().AddInputHandler( &InputHandler );
}

void CConsoleManager::Tick()
{
    if ( IsActive )
    {
        CUIRenderer::DrawUI( []()
        {
            GConsole.DrawInterface();
        } );
    }
}

void CConsoleManager::RegisterCommand( const CString& Name, IConsoleCommand* Command )
{
    // TODO: This feels hacky, fix
    IConsoleObject* Object = reinterpret_cast<IConsoleObject*>(Command);
    if ( !RegisterObject( Name, Object ) )
    {
        LOG_WARNING( "ConsoleCommand '" + Name + "' is already registered" );
    }
}

void CConsoleManager::RegisterVariable( const CString& Name, IConsoleVariable* Variable )
{
    IConsoleObject* Object = reinterpret_cast<IConsoleObject*>(Variable);
    if ( !RegisterObject( Name, Object ) )
    {
        LOG_WARNING( "ConsoleVariable '" + Name + "' is already registered" );
    }
}

IConsoleCommand* CConsoleManager::FindCommand( const CString& Name )
{
    IConsoleObject* Object = FindConsoleObject( Name );
    if ( !Object )
    {
        LOG_ERROR( "Could not find ConsoleCommand '" + Name + '\'' );
        return nullptr;
    }

    IConsoleCommand* Command = Object->AsCommand();
    if ( !Command )
    {
        LOG_ERROR( '\'' + Name + "'Is not a ConsoleCommand'" );
        return nullptr;
    }
    else
    {
        return Command;
    }
}

IConsoleVariable* CConsoleManager::FindVariable( const CString& Name )
{
    IConsoleObject* Object = FindConsoleObject( Name );
    if ( !Object )
    {
        LOG_ERROR( "Could not find ConsoleVariable '" + Name + '\'' );
        return nullptr;
    }

    IConsoleVariable* Variable = Object->AsVariable();
    if ( !Variable )
    {
        LOG_ERROR( '\'' + Name + "'Is not a ConsoleVariable'" );
        return nullptr;
    }
    else
    {
        return Variable;
    }
}

void CConsoleManager::PrintMessage( const CString& Message )
{
    Lines.Emplace( Message, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
}

void CConsoleManager::PrintWarning( const CString& Message )
{
    Lines.Emplace( Message, ImVec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
}

void CConsoleManager::PrintError( const CString& Message )
{
    Lines.Emplace( Message, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
}

void CConsoleManager::ClearHistory()
{
    History.Clear();
    HistoryIndex = -1;
}

void CConsoleManager::OnKeyPressedEvent( const SKeyEvent& Event )
{
    InputHandler.ConsoleActivated = false;

    if ( Event.IsDown )
    {
        if ( !Event.IsRepeat && Event.KeyCode == EKey::Key_GraveAccent )
        {
            IsActive = !IsActive;
            InputHandler.ConsoleActivated = true;
        }
    }
}

void CConsoleManager::DrawInterface()
{
    const uint32 WindowWidth = GEngine->MainWindow->GetWidth();
    const uint32 WindowHeight = GEngine->MainWindow->GetHeight();
    const ImVec2 WindowPadding( 4.0f, 1.0f );
    const float Width  = WindowWidth - (WindowPadding.x * 4.0f);
    const float Height = 200;
    const ImVec2 Offset( 8.0f, 0.0f );

    ImGui::PushStyleColor( ImGuiCol_ResizeGrip, 0 );
    ImGui::PushStyleColor( ImGuiCol_ResizeGripHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ResizeGripActive, 0 );

    ImGui::SetNextWindowPos( ImVec2( Offset.x + 0.0f, Offset.y  ), ImGuiCond_Always, ImVec2( 0.0f, 0.0f ) );
    ImGui::SetNextWindowSize( ImVec2( Width, Height ), ImGuiCond_Always );

    const ImGuiWindowFlags StyleFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin( "Console Window", nullptr, StyleFlags );

    ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, ImVec4( 0.3f, 0.3f, 0.3f, 0.6f ) );

    const ImVec2 ParentSize = ImGui::GetWindowSize();
    const float TextWindowWidth = Width * 0.985f;
    const float TextWindowHeight = ParentSize.y - 40.0f;

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

        float ColumnWidth = 0.0f;
        for ( const SCandidate& Candidate : Candidates )
        {
            if ( Candidate.TextSize.x > ColumnWidth )
            {
                ColumnWidth = Candidate.TextSize.x;
            }
        }

        ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0.4f, 0.4f, 0.4f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0.2f, 0.2f, 0.2f, 1.0f ) );

        for ( int32 i = 0; i < (int32)Candidates.Size(); i++ )
        {
            const SCandidate& Candidate = Candidates[i];
            IsActiveIndex = (CandidatesIndex == i);

            ImGui::PushID( i );
            if ( ImGui::Selectable( Candidate.Text.CStr(), &IsActiveIndex ) )
            {
                strcpy( TextBuffer.Data(), Candidate.Text.CStr() );
                PopupSelectedText = Candidate.Text;

                Candidates.Clear();
                CandidatesIndex = -1;

                UpdateCursorPosition = true;

                ImGui::PopID();
                break;
            }

            ImGui::SameLine( ColumnWidth );

            ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ) );
            ImGui::Text( "%s", Candidate.PostFix.CStr() );
            ImGui::PopStyleColor();

            ImGui::PopID();

            if ( IsActiveIndex && GConsole.CandidateSelectionChanged )
            {
                ImGui::SetScrollHere();
                GConsole.PopupSelectedText = Candidate.Text;
                GConsole.CandidateSelectionChanged = false;
            }
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::PopAllowKeyboardFocus();
        ImGui::PopStyleVar();
    }
    else
    {
        for ( const SLine& Text : Lines )
        {
            ImGui::TextColored( Text.Color, "%s", Text.String.CStr() );
        }

        if ( ScrollDown )
        {
            ImGui::SetScrollHereY();
            ScrollDown = false;
        }
    }

    ImGui::EndChild();

    ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

    ImGui::Text( ">" );
    ImGui::SameLine();

    ImGui::PushItemWidth( TextWindowWidth - 25.0f );

    const ImGuiInputTextFlags InputFlags =
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory |
        ImGuiInputTextFlags_CallbackAlways |
        ImGuiInputTextFlags_CallbackEdit;

    auto Callback = []( ImGuiInputTextCallbackData* Data )->int32
    {
        CConsoleManager* This = reinterpret_cast<CConsoleManager*>(Data->UserData);
        return This->TextCallback( Data );
    };

    const bool Result = ImGui::InputText( "###Input", TextBuffer.Data(), TextBuffer.Size(), InputFlags, Callback, reinterpret_cast<void*>(&GConsole) );
    if ( Result && TextBuffer[0] != 0 )
    {
        if ( CandidatesIndex != -1 )
        {
            strcpy( TextBuffer.Data(), PopupSelectedText.CStr() );

            Candidates.Clear();
            CandidatesIndex = -1;

            UpdateCursorPosition = true;
        }
        else
        {
            const CString Text = CString( TextBuffer.Data() );
            Execute( Text );

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

    ImGui::End();
}

int32 CConsoleManager::TextCallback( ImGuiInputTextCallbackData* Data )
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

            for ( const auto& Object : ConsoleObjects )
            {
                if ( WordLength <= (int32)Object.first.Length() )
                {
                    const char* Command = Object.first.CStr();
                    int32 d = -1;
                    int32 n = WordLength;

                    const char* CmdIt = Command;
                    const char* WordIt = WordStart;
                    while ( n > 0 && (d = toupper( *WordIt ) - toupper( *CmdIt )) == 0 )
                    {
                        CmdIt++;
                        WordIt++;
                        n--;
                    }

                    if ( d == 0 )
                    {
                        IConsoleObject* ConsoleObject = Object.second;
                        Assert( ConsoleObject != nullptr );

                        if ( ConsoleObject->AsCommand() )
                        {
                            Candidates.Emplace( Object.first, "[Cmd]" );
                        }
                        else
                        {
                            IConsoleVariable* Variable = ConsoleObject->AsVariable();
                            if ( Variable->IsBool() )
                            {
                                Candidates.Emplace( Object.first, "= " + Variable->GetString() + " [Boolean]" );
                            }
                            else if ( Variable->IsInt() )
                            {
                                Candidates.Emplace( Object.first, "= " + Variable->GetString() + " [Integer]" );
                            }
                            else if ( Variable->IsFloat() )
                            {
                                Candidates.Emplace( Object.first, "= " + Variable->GetString() + " [float]" );
                            }
                            else if ( Variable->IsString() )
                            {
                                Candidates.Emplace( Object.first, "= " + Variable->GetString() + " [String]" );
                            }
                        }
                    }
                }
            }

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
                    Data->InsertChars( Data->CursorPos, Candidates[0].Text.CStr() );

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

void CConsoleManager::Execute( const CString& CmdString )
{
    PrintMessage( CmdString );

    // Erase history
    History.Emplace( CmdString );
    if ( History.Size() > (int32)HistoryLength )
    {
        History.RemoveAt( History.StartIterator() );
    }

    int32 Pos = CmdString.FindOneOf( " " );
    if ( Pos == CString::InvalidPosition )
    {
        IConsoleCommand* Command = FindCommand( CmdString );
        if ( !Command )
        {
            const CString Message = "'" + CmdString + "' is not a registered command";
            PrintError( Message );
        }
        else
        {
            Command->Execute();
        }
    }
    else
    {
        CString VariableName( CmdString.CStr(), Pos );

        IConsoleVariable* Variable = FindVariable( VariableName );
        if ( !Variable )
        {
            PrintError( "'" + CmdString + "' is not a registered variable" );
            return;
        }

        Pos++;

        CString Value( CmdString.CStr() + Pos, CmdString.Length() - Pos );
        if ( std::regex_match( Value.CStr(), std::regex( "[-]?[0-9]+" ) ) )
        {
            Variable->SetString( Value );
        }
        else if ( std::regex_match( Value.CStr(), std::regex( "[-]?[0-9]*[.][0-9]+" ) ) && Variable->IsFloat() )
        {
            Variable->SetString( Value );
        }
        else if ( std::regex_match( Value.CStr(), std::regex( "(false)|(true)" ) ) && Variable->IsBool() )
        {
            Variable->SetString( Value );
        }
        else
        {
            if ( Variable->IsString() )
            {
                Variable->SetString( Value );
            }
            else
            {
                const CString Message = "'" + Value + "' Is an invalid value for '" + VariableName + "'";
                PrintError( Message );
            }
        }
    }
}

bool CConsoleManager::RegisterObject( const CString& Name, IConsoleObject* Object )
{
    auto ExistingObject = ConsoleObjects.find( Name );
    if ( ExistingObject == ConsoleObjects.end() )
    {
        ConsoleObjects.insert( std::make_pair( Name, Object ) );
        return true;
    }
    else
    {
        return false;
    }
}

IConsoleObject* CConsoleManager::FindConsoleObject( const CString& Name )
{
    auto ExisitingObject = ConsoleObjects.find( Name );
    if ( ExisitingObject != ConsoleObjects.end() )
    {
        return ExisitingObject->second;
    }
    else
    {
        return nullptr;
    }
}
