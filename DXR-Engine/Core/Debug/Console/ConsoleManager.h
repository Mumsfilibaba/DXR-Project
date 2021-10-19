#pragma once
#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include "Core/Application/Events.h"
#include "Core/Application/InputHandler.h"
#include "Core/Containers/HashTable.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define INIT_CONSOLE_VARIABLE(Name, Variable) CConsoleManager::Get().RegisterVariable(Name, Variable)
#define INIT_CONSOLE_COMMAND(Name, Command)   CConsoleManager::Get().RegisterCommand(Name, Command)

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

class CORE_API CConsoleManager
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

    /* Init the console */
    static void Init();

    static FORCEINLINE CConsoleManager& Get()
    {
        return Instance;
    }

    /* Tick the console */
    void Tick();

    /* Register a console command */
    void RegisterCommand( const CString& Name, IConsoleCommand* Object );
    
    /* Register a console variable */
    void RegisterVariable( const CString& Name, IConsoleVariable* Variable );

    /* Finds a console-command, returns nullptr otherwise, including if the object is a variable */
    IConsoleCommand* FindCommand( const CString& Name );
    
    /* Finds a console-variable, returns nullptr otherwise, including if the object is a command */
    IConsoleVariable* FindVariable( const CString& Name );

    /* Print messages with different severity */
    void PrintMessage( const CString& Message );
    void PrintWarning( const CString& Message );
    void PrintError( const CString& Message );

    /* Clears the console history */
    void ClearHistory();

private:

    /* Called when a key is pressed */
    void OnKeyPressedEvent( const SKeyEvent& Event );

    /* Draws the console interface */
    void DrawInterface();

    /* Registers a new console object */
    bool RegisterObject( const CString& Name, IConsoleObject* Variable );

    /* Returns a console object if the it exists, otherwise it returns nullptr */
    IConsoleObject* FindConsoleObject( const CString& Name );

    /* Callback from the input */
    int32 TextCallback( ImGuiInputTextCallbackData* Data );

    /* Execute a string from the console */
    void Execute( const CString& CmdString );

    /* All console objects */
    THashTable<CString, IConsoleObject*, SStringHasher> ConsoleObjects;

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

    static CConsoleManager Instance;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
