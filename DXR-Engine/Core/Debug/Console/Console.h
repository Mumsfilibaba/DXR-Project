#pragma once
#include "Core/Application/Events.h"
#include "Core/Application/InputHandler.h"

#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include <unordered_map>

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define INIT_CONSOLE_VARIABLE(Name, Variable) GConsole.RegisterVariable(Name, Variable)
#define INIT_CONSOLE_COMMAND(Name, Command)   GConsole.RegisterCommand(Name, Command)

class CConsoleInputHandler : public CInputHandler
{
public:
	CConsoleInputHandler() = default;
	~CConsoleInputHandler() = default;
	
	virtual bool HandleKeyEvent( const SKeyEvent& KeyEvent )
	{
		HandleKeyEventDelegate.Execute( KeyEvent );
		return true;
	}
	
	DECLARE_DELEGATE( CHandleKeyEventDelegate, const SKeyEvent& );
	CHandleKeyEventDelegate HandleKeyEventDelegate;
};


class Console
{
    struct Line
    {
        Line() = default;

        Line( const String& InString, ImVec4 InColor )
            : String( InString )
            , Color( InColor )
        {
        }

        String String;
        ImVec4 Color;
    };

    struct Candidate
    {
        static constexpr float TextPadding = 20.0f;

        Candidate() = default;

        Candidate( const String& InText, const String& InPostFix )
            : Text( InText )
            , PostFix( InPostFix )
        {
            TextSize = ImGui::CalcTextSize( Text.c_str() );
            TextSize.x += TextPadding;
        }

        String Text;
        String PostFix;
        ImVec2 TextSize;
    };

public:
    void Init();
    void Tick();

    void RegisterCommand( const String& Name, ConsoleCommand* Object );
    void RegisterVariable( const String& Name, ConsoleVariable* Variable );

    ConsoleCommand* FindCommand( const String& Name );
    ConsoleVariable* FindVariable( const String& Name );

    void PrintMessage( const String& Message );
    void PrintWarning( const String& Message );
    void PrintError( const String& Message );

    void ClearHistory();

private:
    void OnKeyPressedEvent( const SKeyEvent& Event );

    void DrawInterface();

    bool RegisterObject( const String& Name, ConsoleObject* Variable );

    ConsoleObject* FindConsoleObject( const String& Name );

    int32 TextCallback( ImGuiInputTextCallbackData* Data );

    void Execute( const String& CmdString );

private:
    std::unordered_map<String, ConsoleObject*> ConsoleObjects;

    String PopupSelectedText;

    TArray<Candidate> Candidates;
    int32 CandidatesIndex = -1;

    TStaticArray<char, 256> TextBuffer;

    TArray<Line> Lines;

    TArray<String> History;
    uint32 HistoryLength = 50;
    int32  HistoryIndex = -1;

    bool UpdateCursorPosition = false;
    bool CandidateSelectionChanged = false;
    bool ScrollDown = false;
    bool IsActive = false;
	
	CConsoleInputHandler InputHandler;
};

extern Console GConsole;

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
