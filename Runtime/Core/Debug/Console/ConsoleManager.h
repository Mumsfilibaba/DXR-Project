#pragma once
#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include "Application/Events.h"
#include "Application/InputHandler.h"

#include "Core/Containers/HashTable.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/StringView.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Severity of the console-message

enum class EConsoleSeverity
{
    Info = 0,
    Warning = 1,
    Error = 2
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleManager - Manages ConsoleObjects

class CORE_API CConsoleManager
{
public:

    /**
     * Retrieve the ConsoleManager instance
     * 
     * @return: Returns a reference to the ConsoleManager
     */
    static CConsoleManager& Get();

    /**
     * Register a new console-command
     * 
     * @param Name: Name of the console-command
     * @param Command: Command to register
     */
    void RegisterCommand(const String& Name, IConsoleCommand* Command);
    
    /**
     * Register a new console-variable
     *
     * @param Name: Name of the console-variable
     * @param Variable: variable to register
     */
    void RegisterVariable(const String& Name, IConsoleVariable* Variable);

    /**
     * Unregister a console-object
     * 
     * @param Name: Name of the console-object to unregister from the console manager
     */
    void UnregisterObject(const String& Name);

    /**
     * Check weather or not a console-object exists with a specific name
     * 
     * @param Name: Name of the console-object
     * @return: Returns true if there exists a console-object with the specified name
     */
    bool IsConsoleObject(const String& Name) const;

    /**
     * Finds a console-command, returns nullptr otherwise, including if the object is a variable
     * 
     * @param Name: Name of the console-command
     * @return: The console-command matching the name
     */
    IConsoleCommand* FindCommand(const String& Name);

    /**
     * Finds a console-variable, returns nullptr otherwise, including if the object is a command
     *
     * @param Name: Name of the console-variable
     * @return: The console-variable matching the name
     */
    IConsoleVariable* FindVariable(const String& Name);

    /**
     * Retrieve all ConsoleObjects that fits the name of the specified string
     * 
     * @param CandidateName: Names to match
     * @param OutCandidates: Array to store the console-objects that matches the candidate-name
     */
    void FindCandidates(const StringView& CandidateName, TArray<TPair<IConsoleObject*, String>>& OutCandidates);

    /**
     * Print a message to the console
     * 
     * @param Message: Message to print to the console
     * @param Severity: Severity of the message
     */
    void PrintMessage(const String& Message, EConsoleSeverity Severity);

    /** Clears the console history */
    void ClearHistory();

    /**
     * Execute a string from the console 
     * 
     * @param Command: Command to execute by the console
     */
    void Execute(const String& Command);

    /**
     * Retrieve all the messages printed to the console
     * 
     * @return: An array containing pairs of the console-messages and console-severity that has been printed to the console.
     */
    FORCEINLINE const TArray<TPair<String, EConsoleSeverity>>& GetMessages() const 
    { 
        return ConsoleMessages;
    }

    /**
     * Retrieve all the history that has been written to the console 
     * 
     * @return: An array containing string of all history written to the console
     */
    FORCEINLINE const TArray<String>& GetHistory() const 
    { 
        return History; 
    }

private:

    CConsoleManager() = default;
    ~CConsoleManager() = default;

    bool RegisterObject(const String& Name, IConsoleObject* Variable);

    IConsoleObject* FindConsoleObject(const String& Name);

private:

    THashTable<String, IConsoleObject*, SStringHasher> ConsoleObjects;

    TArray<TPair<String, EConsoleSeverity>> ConsoleMessages;

    TArray<String> History;
    int32 HistoryLength = 50;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// AutoConsoleCommand - Console-command that registers itself to the console-manager

class CAutoConsoleCommand : public CConsoleCommand
{
public:
    CAutoConsoleCommand(const String& InName)
        : CConsoleCommand()
        , Name(InName)
    {
        CConsoleManager::Get().RegisterCommand(InName, this);
    }
    
    CAutoConsoleCommand(const String& InName, const CExecutedDelegateType& Delegate)
        : CConsoleCommand(Delegate)
        , Name(InName)
    {
        CConsoleManager::Get().RegisterCommand(InName, this);
    }

    ~CAutoConsoleCommand()
    {
        CConsoleManager::Get().UnregisterObject(Name);
    }

private:
    String Name;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// AutoConsoleVariable - Console-variable that registers itself to the console-manager

template<typename T>
class TAutoConsoleVariable : public TConsoleVariable<T>
{
public:
    TAutoConsoleVariable(const String& InName, T StartValue)
        : TConsoleVariable<T>(StartValue)
        , Name(InName)
    {
        CConsoleManager::Get().RegisterVariable(InName, this);
    }

    TAutoConsoleVariable(const String& InName, T StartValue, const CConsoleVariableChangedDelegateType& VariableChangedDelegate)
        : TConsoleVariable<T>(StartValue, VariableChangedDelegate)
        , Name(InName)
    {
        CConsoleManager::Get().RegisterVariable(InName, this);
    }

    ~TAutoConsoleVariable()
    {
        CConsoleManager::Get().UnregisterObject(Name);
    }

private:
    String Name;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
