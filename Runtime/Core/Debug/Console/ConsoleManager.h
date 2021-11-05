#pragma once
#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include "Interface/Events.h"
#include "Interface/InputHandler.h"

#include "Core/Containers/HashTable.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/StringView.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define INIT_CONSOLE_VARIABLE(Name, Variable) CConsoleManager::Get().RegisterVariable(Name, Variable)
#define INIT_CONSOLE_COMMAND(Name, Command)   CConsoleManager::Get().RegisterCommand(Name, Command)

enum class EConsoleSeverity
{
    Info = 0,
    Warning = 1,
    Error = 2
};

class CORE_API CConsoleManager
{
public:

    /* Init the console */
    static void Init();

    static FORCEINLINE CConsoleManager& Get()
    {
        return Instance;
    }

    /* Register a console command */
    void RegisterCommand( const CString& Name, IConsoleCommand* Object );

    /* Register a console variable */
    void RegisterVariable( const CString& Name, IConsoleVariable* Variable );

    /* Finds a console-command, returns nullptr otherwise, including if the object is a variable */
    IConsoleCommand* FindCommand( const CString& Name );

    /* Finds a console-variable, returns nullptr otherwise, including if the object is a command */
    IConsoleVariable* FindVariable( const CString& Name );

    /* Retrieve all ConsoleObjects that fits the name of the specified string */
    void FindCandidates( const CStringView& CandidateName, TArray<TPair<IConsoleObject*, CString>>& OutCandidates );

    /* Print messages with different severity */
    void PrintMessage( const CString& Message, EConsoleSeverity Severity );

    /* Clears the console history */
    void ClearHistory();

    /* Execute a string from the console */
    void Execute( const CString& CmdString );

    /* Get all the messages printed to the console */
    FORCEINLINE const TArray<TPair<CString, EConsoleSeverity>>& GetMessages() const
    {
        return ConsoleMessages;
    }

    FORCEINLINE const TArray<CString>& GetHistory() const
    {
        return History;
    }

private:

    CConsoleManager() = default;
    ~CConsoleManager() = default;

    /* Registers a new console object */
    bool RegisterObject( const CString& Name, IConsoleObject* Variable );

    /* Returns a console object if the it exists, otherwise it returns nullptr */
    IConsoleObject* FindConsoleObject( const CString& Name );

    /* All console objects */
    THashTable<CString, IConsoleObject*, SStringHasher> ConsoleObjects;

    /* All messages printed via the print message functions together with the severity */
    TArray<TPair<CString, EConsoleSeverity>> ConsoleMessages;

    /* The history of all the executed strings */
    TArray<CString> History;
    int32 HistoryLength = 50;

    static CConsoleManager Instance;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
