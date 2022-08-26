#pragma once
#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include "Application/Events.h"
#include "Application/InputHandler.h"

#include "Core/Containers/Map.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/Optional.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EConsoleSeverity

enum class EConsoleSeverity
{
    Info    = 0,
    Warning = 1,
    Error   = 2
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FConsoleInterface

class CORE_API FConsoleInterface
{
protected:
    FConsoleInterface()  = default;
    ~FConsoleInterface() = default;

public:

    /**
     * @brief: Retrieve the ConsoleManager instance
     * 
     * @return: Returns a reference to the ConsoleManager
     */
    static FConsoleInterface& Get();

    /**
     * @brief: Register a new console-command
     * 
     * @param Name: Name of the console-command
     * @param Command: Command to register
     */
    virtual void RegisterCommand(const FString& Name, IConsoleCommand* Command) = 0;
    
    /**
     * @brief: Register a new console-variable
     *
     * @param Name: Name of the console-variable
     * @param Variable: variable to register
     */
    virtual void RegisterVariable(const FString& Name, IConsoleVariable* Variable) = 0;

    /**
     * @brief: Unregister a console-object
     * 
     * @param Name: Name of the console-object to unregister from the console manager
     */
    virtual void UnregisterObject(const FString& Name) = 0;

    /**
     * @brief: Check weather or not a console-object exists with a specific name
     * 
     * @param Name: Name of the console-object
     * @return: Returns true if there exists a console-object with the specified name
     */
    virtual bool IsConsoleObject(const FString& Name) const = 0;

    /**
     * @brief: Finds a console-command, returns nullptr otherwise, including if the object is a variable
     * 
     * @param Name: Name of the console-command
     * @return: The console-command matching the name
     */
    virtual IConsoleCommand* FindCommand(const FString& Name) = 0;

    /**
     * @brief: Finds a console-variable, returns nullptr otherwise, including if the object is a command
     *
     * @param Name: Name of the console-variable
     * @return: The console-variable matching the name
     */
    virtual IConsoleVariable* FindVariable(const FString& Name) = 0;

    /**
     * @brief: Retrieve all ConsoleObjects that fits the name of the specified string
     * 
     * @param CandidateName: Names to match
     * @param OutCandidates: Array to store the console-objects that matches the candidate-name
     */
    virtual void FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates) = 0;

    /**
     * @brief: Print a message to the console
     * 
     * @param Message: Message to print to the console
     * @param Severity: Severity of the message
     */
    virtual void PrintMessage(const FString& Message, EConsoleSeverity Severity) = 0;

    /** 
     * @brief: Clears the console history 
     */
    virtual void ClearHistory() = 0;

    /**
     * @brief: Execute a string from the console 
     * 
     * @param Command: Command to execute by the console
     */
    virtual void Execute(const FString& Command) = 0;

    /**
     * @brief: Retrieve all the messages printed to the console
     * 
     * @return: An array containing pairs of the console-messages and console-severity that has been printed to the console.
     */
    virtual const TArray<TPair<FString, EConsoleSeverity>>& GetMessages() const = 0;

    /**
     * @brief: Retrieve all the history that has been written to the console 
     * 
     * @return: An array containing string of all history written to the console
     */
    virtual const TArray<FString>& GetHistory() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAutoConsoleCommand

class FAutoConsoleCommand
    : public FConsoleCommand
{
public:
    FAutoConsoleCommand(const FString& InName)
        : FConsoleCommand()
        , Name(InName)
    {
        FConsoleInterface& ConsoleManager = FConsoleInterface::Get();
        ConsoleManager.RegisterCommand(InName, this);
    }
    
    FAutoConsoleCommand(const FString& InName, const FCommandDelegateType& Delegate)
        : FConsoleCommand(Delegate)
        , Name(InName)
    {
        FConsoleInterface& ConsoleManager = FConsoleInterface::Get();
        ConsoleManager.RegisterCommand(InName, this);
    }

    ~FAutoConsoleCommand()
    {
        FConsoleInterface& ConsoleManager = FConsoleInterface::Get();
        ConsoleManager.UnregisterObject(Name);
    }

private:
    FString Name;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAutoConsoleVariable

template<typename T>
class TAutoConsoleVariable 
    : public TConsoleVariable<T>
{
public:
    TAutoConsoleVariable(const FString& InName, T StartValue)
        : TConsoleVariable<T>(StartValue)
        , Name(InName)
    {
        FConsoleInterface& ConsoleManager = FConsoleInterface::Get();
        ConsoleManager.RegisterVariable(InName, this);
    }

    TAutoConsoleVariable(const FString& InName, T StartValue, const FCVarChangedDelegateType& VariableChangedDelegate)
        : TConsoleVariable<T>(StartValue, VariableChangedDelegate)
        , Name(InName)
    {
        FConsoleInterface& ConsoleManager = FConsoleInterface::Get();
        ConsoleManager.RegisterVariable(InName, this);
    }

    ~TAutoConsoleVariable()
    {
        FConsoleInterface& ConsoleManager = FConsoleInterface::Get();
        ConsoleManager.UnregisterObject(Name);
    }

private:
    FString Name;
};
