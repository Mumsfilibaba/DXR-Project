#pragma once
#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include "Canvas/Events.h"
#include "Canvas/InputHandler.h"

#include "Core/Containers/HashTable.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/Optional.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EConsoleSeverity

enum class EConsoleSeverity
{
    Info    = 0,
    Warning = 1,
    Error   = 2
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FConsoleManager

class CORE_API FConsoleManager
{
private:

    friend class TOptional<FConsoleManager>;

    FConsoleManager()  = default;
    ~FConsoleManager() = default;

public:

    /**
     * @brief: Retrieve the ConsoleManager instance
     * 
     * @return: Returns a reference to the ConsoleManager
     */
    static FConsoleManager& Get();

    /**
     * @brief: Register a new console-command
     * 
     * @param Name: Name of the console-command
     * @param Command: Command to register
     */
    void RegisterCommand(const FString& Name, IConsoleCommand* Command);
    
    /**
     * @brief: Register a new console-variable
     *
     * @param Name: Name of the console-variable
     * @param Variable: variable to register
     */
    void RegisterVariable(const FString& Name, IConsoleVariable* Variable);

    /**
     * @brief: Unregister a console-object
     * 
     * @param Name: Name of the console-object to unregister from the console manager
     */
    void UnregisterObject(const FString& Name);

    /**
     * @brief: Check weather or not a console-object exists with a specific name
     * 
     * @param Name: Name of the console-object
     * @return: Returns true if there exists a console-object with the specified name
     */
    bool IsConsoleObject(const FString& Name) const;

    /**
     * @brief: Finds a console-command, returns nullptr otherwise, including if the object is a variable
     * 
     * @param Name: Name of the console-command
     * @return: The console-command matching the name
     */
    IConsoleCommand* FindCommand(const FString& Name);

    /**
     * @brief: Finds a console-variable, returns nullptr otherwise, including if the object is a command
     *
     * @param Name: Name of the console-variable
     * @return: The console-variable matching the name
     */
    IConsoleVariable* FindVariable(const FString& Name);

    /**
     * @brief: Retrieve all ConsoleObjects that fits the name of the specified string
     * 
     * @param CandidateName: Names to match
     * @param OutCandidates: Array to store the console-objects that matches the candidate-name
     */
    void FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates);

    /**
     * @brief: Print a message to the console
     * 
     * @param Message: Message to print to the console
     * @param Severity: Severity of the message
     */
    void PrintMessage(const FString& Message, EConsoleSeverity Severity);

    /** Clears the console history */
    void ClearHistory();

    /**
     * @brief: Execute a string from the console 
     * 
     * @param Command: Command to execute by the console
     */
    void Execute(const FString& Command);

    /**
     * @brief: Retrieve all the messages printed to the console
     * 
     * @return: An array containing pairs of the console-messages and console-severity that has been printed to the console.
     */
    FORCEINLINE const TArray<TPair<FString, EConsoleSeverity>>& GetMessages() const 
    { 
        return ConsoleMessages;
    }

    /**
     * @brief: Retrieve all the history that has been written to the console 
     * 
     * @return: An array containing string of all history written to the console
     */
    FORCEINLINE const TArray<FString>& GetHistory() const 
    { 
        return History; 
    }

private:
    
    static TOptional<FConsoleManager>& GetConsoleManagerInstance();
    
    bool RegisterObject(const FString& Name, IConsoleObject* Variable);

    IConsoleObject* FindConsoleObject(const FString& Name);

private:

    THashTable<FString, IConsoleObject*, FStringHasher> ConsoleObjects;

    TArray<TPair<FString, EConsoleSeverity>> ConsoleMessages;

    TArray<FString> History;
    int32           HistoryLength = 50;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAutoConsoleCommand

class FAutoConsoleCommand : public FConsoleCommand
{
public:
    FAutoConsoleCommand(const FString& InName)
        : FConsoleCommand()
        , Name(InName)
    {
        FConsoleManager::Get().RegisterCommand(InName, this);
    }
    
    FAutoConsoleCommand(const FString& InName, const FCommandDelegateType& Delegate)
        : FConsoleCommand(Delegate)
        , Name(InName)
    {
        FConsoleManager::Get().RegisterCommand(InName, this);
    }

    ~FAutoConsoleCommand()
    {
        FConsoleManager::Get().UnregisterObject(Name);
    }

private:
    FString Name;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAutoConsoleVariable

template<typename T>
class TAutoConsoleVariable : public TConsoleVariable<T>
{
public:
    TAutoConsoleVariable(const FString& InName, T StartValue)
        : TConsoleVariable<T>(StartValue)
        , Name(InName)
    {
        FConsoleManager::Get().RegisterVariable(InName, this);
    }

    TAutoConsoleVariable(const FString& InName, T StartValue, const FCVarChangedDelegateType& VariableChangedDelegate)
        : TConsoleVariable<T>(StartValue, VariableChangedDelegate)
        , Name(InName)
    {
        FConsoleManager::Get().RegisterVariable(InName, this);
    }

    ~TAutoConsoleVariable()
    {
        FConsoleManager::Get().UnregisterObject(Name);
    }

private:
    FString Name;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
