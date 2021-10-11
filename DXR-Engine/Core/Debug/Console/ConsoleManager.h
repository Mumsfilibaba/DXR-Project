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

class CConsoleInputHandler final : public CInputHandler
{
public:

    DECLARE_DELEGATE( CHandleKeyEventDelegate, const SKeyEvent& );
    CHandleKeyEventDelegate HandleKeyEventDelegate;

    CConsoleInputHandler() = default;
    ~CConsoleInputHandler() = default;

    virtual bool HandleKeyEvent( const SKeyEvent& KeyEvent ) override final
    {
        HandleKeyEventDelegate.Execute( KeyEvent );
        return ConsoleActivated;
    }

    virtual uint32 GetPriority() const override final
    {
        return static_cast<uint32>(-1);
    }

    bool ConsoleActivated = false;
};

class CConsoleManager
{
    struct SLine
    {
        SLine() = default;

        FORCEINLINE SLine( const CString& InString, ImVec4 InColor )
            : String( InString )
            , Color( InColor )
        {
        }

        CString String;
        ImVec4 Color;
    };

    struct SCandidate
    {
        static constexpr float TextPadding = 20.0f;

        SCandidate() = default;

        FORCEINLINE SCandidate( const CString& InText, const CString& InPostFix )
            : Text( InText )
            , PostFix( InPostFix )
        {
            TextSize = ImGui::CalcTextSize( Text.CStr() );
            TextSize.x += TextPadding;
        }

        CString Text;
        CString PostFix;
        ImVec2 TextSize;
    };

public:
    void Init();
    void Tick();

    void RegisterCommand( const CString& Name, CConsoleCommand* Object );
    void RegisterVariable( const CString& Name, CConsoleVariable* Variable );

    CConsoleCommand* FindCommand( const CString& Name );
    CConsoleVariable* FindVariable( const CString& Name );

    void PrintMessage( const CString& Message );
    void PrintWarning( const CString& Message );
    void PrintError( const CString& Message );

    void ClearHistory();

private:
    void OnKeyPressedEvent( const SKeyEvent& Event );

    void DrawInterface();

    bool RegisterObject( const CString& Name, CConsoleObject* Variable );

    CConsoleObject* FindConsoleObject( const CString& Name );

    int32 TextCallback( ImGuiInputTextCallbackData* Data );

    void Execute( const CString& CmdString );

private:
    std::unordered_map<CString, CConsoleObject*, SStringHasher> ConsoleObjects;

    CString PopupSelectedText;

    TArray<SCandidate> Candidates;
    int32 CandidatesIndex = -1;

    TStaticArray<char, 256> TextBuffer;

    TArray<SLine> Lines;

    TArray<CString> History;
    uint32 HistoryLength = 50;
    int32  HistoryIndex = -1;

    bool UpdateCursorPosition = false;
    bool CandidateSelectionChanged = false;
    bool ScrollDown = false;
    bool IsActive = false;

    CConsoleInputHandler InputHandler;
};

extern CConsoleManager GConsole;

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
