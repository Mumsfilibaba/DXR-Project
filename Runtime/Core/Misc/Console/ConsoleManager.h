#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/AutoPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/MulticastDelegate.h"

DECLARE_DELEGATE(FConsoleCommandDelegate);
DECLARE_DELEGATE(FConsoleVariableDelegate, struct IConsoleVariable*);


struct IConsoleObject
{
    virtual ~IConsoleObject() = default;

    /**
     * @brief  - Cast to a console-variable if the console-variable interface is implemented
     * @return - Returns either a console-variable or nullptr
     */
    virtual struct IConsoleVariable* AsVariable() = 0;

    /**
     * @brief  - Cast to a console-command if the console-command interface is implemented
     * @return - Returns either a console-command or nullptr
     */
    virtual struct IConsoleCommand* AsCommand() = 0;
};


struct IConsoleCommand
    : public IConsoleObject
{
    // TODO: Add parameters to console commands

    /**
     * @brief - Execute the ConsoleCommand
     */
    virtual void Execute() = 0;
};


template<typename T>
class TConsoleVariableData;

struct IConsoleVariable
    : public IConsoleObject
{
    /**
     * @brief         - Set the variable with an int
     * @param InValue - Value to store
     */
    virtual void SetAsInt(int32 InValue) = 0;

    /**
     * @brief         - Set the variable with an float
     * @param InValue - Value to store
     */
    virtual void SetAsFloat(float InValue) = 0;

    /**
     * @brief        - Set the variable with an bool
     * @param bValue - Value to store
     */
    virtual void SetAsBool(bool bValue) = 0;

    /**
     * @brief         - Set the variable with an string
     * @param InValue - Value to store
     */
    virtual void SetString(const FString& InValue) = 0;

    /**
     * @brief  - Retrieve the variable as an int
     * @return - Returns an int with the value of the variable
     */
    virtual int32 GetInt() const = 0;

    /**
     * @brief  - Retrieve the variable as an float
     * @return - Returns an float with the value of the variable
     */
    virtual float GetFloat() const = 0;

    /**
     * @brief  - Retrieve the variable as an bool
     * @return - Returns an bool with the value of the variable
     */
    virtual bool GetBool() const = 0;

    /**
     * @brief  - Retrieve the variable as an string
     * @return - Returns an string with the value of the variable
     */
    virtual FString GetString() const = 0;

    /**
     * @brief  - Retrieve the variable's data as a pointer
     * @return - Returns an pointer to int-data if this is a int variable otherwise nullptr
     */
    virtual TConsoleVariableData<int32>* GetIntData() = 0;

    /**
     * @brief  - Retrieve the variable's datacontainer as a pointer
     * @return - Returns an pointer to float-datacontainer if this is a float variable otherwise nullptr
     */
    virtual TConsoleVariableData<float>* GetFloatData() = 0;

    /**
     * @brief  - Retrieve the variable's datacontainer as a pointer
     * @return - Returns an pointer to bool-datacontainer if this is a bool variable otherwise nullptr
     */
    virtual TConsoleVariableData<bool>* GetBoolData() = 0;

    /**
     * @brief  - Retrieve the variable's datacontainer as a pointer
     * @return - Returns an pointer to string-datacontainer if this is a string variable otherwise nullptr
     */
    virtual TConsoleVariableData<FString>* GetStringData() = 0;

    /**
     * @brief  - Check weather the variable is an int
     * @return - Returns true if the variable is an int
     */
    virtual bool IsVariableInt() const = 0;

    /**
     * @brief  - Check weather the variable is a float
     * @return - Returns true if the variable is a float
     */
    virtual bool IsVariableFloat() const = 0;

    /**
     * @brief  - Check weather the variable is a bool
     * @return - Returns true if the variable is a bool
     */
    virtual bool IsVariableBool() const = 0;

    /**
     * @brief  - Check weather the variable is a string
     * @return - Returns true if the variable is a string
     */
    virtual bool IsVariableString() const = 0;

    /**
     * @brief                    - Set the callback for when the variable changes
     * @param NewChangedDelegate - Delegate for when the variable changes 
     */
    virtual void SetOnChangedDelegate(const FConsoleVariableDelegate& NewChangedDelegate) = 0;

    /**
     * @brief  - Retrieve the delegate that gets called when the variable changes
     * @return - Returns the on changed delegate
     */
    virtual FConsoleVariableDelegate& GetOnChangedDelegate() = 0;
};


enum class EConsoleSeverity
{
    Info    = 0,
    Warning = 1,
    Error   = 2
};


class CORE_API FConsoleManager
{
    FConsoleManager()  = default;
    ~FConsoleManager();

public:

    /**
     * @brief  - Retrieve the ConsoleManager instance
     * @return - Returns a reference to the ConsoleManager
     */
    static FORCEINLINE FConsoleManager& Get()
    {
        if (!GInstance)
        {
            CreateConsoleManager();
            CHECK(GInstance != nullptr);
        }

        return *GInstance;
    }

    /**
     * @brief                 - Register a new console-command
     * @param Name            - Name of the console-command
     * @param CommandDelegate - CommandDelegate to call when executing the command
     */
    IConsoleCommand* RegisterCommand(const CHAR* InName, const FConsoleCommandDelegate& CommandDelegate);

    /**
     * @brief              - Register a new String ConsoleVariable
     * @param Name         - Name of the ConsoleVariable
     * @param DefaultValue - Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* DefaultValue);

    /**
     * @brief              - Register a new int32 ConsoleVariable
     * @param Name         - Name of the ConsoleVariable
     * @param DefaultValue - Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, int32 DefaultValue);

    /**
     * @brief              - Register a new float ConsoleVariable
     * @param Name         - Name of the ConsoleVariable
     * @param DefaultValue - Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, float DefaultValue);

    /**
     * @brief               - Register a new bool ConsoleVariable
     * @param Name          - Name of the ConsoleVariable
     * @param bDefaultValue - Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, bool bDefaultValue);

    /**
     * @brief               - Unregister a ConsoleObject
     * @param ConsoleObject - ConsoleObject to unregister from the console manager
     */
    void UnregisterObject(IConsoleObject* ConsoleObject);

    /**
     * @brief      - Check weather or not a console-object exists with a specific name
     * @param Name - Name of the console-object
     * @return     - Returns true if there exists a console-object with the specified name
     */
    bool IsConsoleObject(const CHAR* Name) const;

    /**
     * @brief               - Finds the name of a ConsoleObject
     * @param ConsoleObject - Name of the ConsoleCommand
     * @return              - The ConsoleCommand matching the name
     */
    FString FindConsoleObjectName(IConsoleObject* ConsoleObject);

    /**
     * @brief      - Finds the ConsoleCommand with the matching name, returns nullptr if not found
     * @param Name - Name of the ConsoleCommand
     * @return     - The ConsoleCommand matching the name
     */
    IConsoleCommand* FindConsoleCommand(const CHAR* Name) const;

    /**
     * @brief      - Find the ConsoleVariable with the matching name, returns nullptr if not found
     * @param Name - Name of the ConsoleVariable
     * @return     - The ConsoleVariable matching the name
     */
    IConsoleVariable* FindConsoleVariable(const CHAR* Name) const;

    /**
     * @brief      - Finds a any ConsoleObject with the matching name, returns nullptr if not found
     * @param Name - Name of the ConsoleObject
     * @return     - The ConsoleObject matching the name
     */
    IConsoleObject* FindConsoleObject(const CHAR* Name) const;

    /**
     * @brief               - Retrieve all ConsoleObjects that fits the name of the specified string
     * @param CandidateName - Names to match
     * @param OutCandidates - Array to store the console-objects that matches the candidate-name
     */
    void FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates);

    /**
     * @brief          - Print a message to the console
     * @param Message  - Message to print to the console
     * @param Severity - Severity of the message
     */
    void PrintMessage(const FString& Message, EConsoleSeverity Severity);

    /**
     * @brief - Clears the console history
     */
    void ClearHistory();

    /**
     * @brief         - Execute a string from the console
     * @param Command - Command to execute by the console
     */
    void Execute(const FString& Command);

    /**
     * @brief  - Retrieve all the messages printed to the console
     * @return - An array containing pairs of the console-messages and console-severity that has been printed to the console.
     */
    const TArray<TPair<FString, EConsoleSeverity>>& GetMessages() const
    {
        return ConsoleMessages;
    }

    /**
     * @brief  - Retrieve all the history that has been written to the console
     * @return - An array containing string of all history written to the console
     */
    const TArray<FString>& GetHistory() const
    {
        return History;
    }

private:
    static void CreateConsoleManager();

    IConsoleObject* RegisterObject(const CHAR* Name, IConsoleObject* Variable);

    TMap<FString, IConsoleObject*, FStringHasher> ConsoleObjects;
    TArray<TPair<FString, EConsoleSeverity>>      ConsoleMessages;

    TArray<FString> History;
    int32           HistoryLength = 50;

    static FConsoleManager* GInstance;
};


class FAutoConsoleObject
{
public: 
    FAutoConsoleObject(FAutoConsoleObject&&)      = delete;
    FAutoConsoleObject(const FAutoConsoleObject&) = delete;
    FAutoConsoleObject& operator=(FAutoConsoleObject&&)      = delete;
    FAutoConsoleObject& operator=(const FAutoConsoleObject&) = delete;

    FAutoConsoleObject(IConsoleObject* ConsoleObject)
        : ConsoleObject(ConsoleObject)
    { 
        CHECK(ConsoleObject != nullptr);
    }

    virtual ~FAutoConsoleObject()
    {
        CHECK(ConsoleObject != nullptr);
        FConsoleManager::Get().UnregisterObject(ConsoleObject);
    }

    IConsoleVariable* AsVariable() const
    {
        return ConsoleObject->AsVariable();
    }

    IConsoleCommand* AsCommand() const
    {
        return ConsoleObject->AsCommand();
    }

private:
    IConsoleObject* ConsoleObject;
};


class FAutoConsoleCommand
    : public FAutoConsoleObject
{
public:
    FAutoConsoleCommand(const CHAR* InName, const FConsoleCommandDelegate& Delegate)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterCommand(InName, Delegate))
    { }

    FORCEINLINE IConsoleCommand& operator*()
    {
        return *AsCommand();
    }

    FORCEINLINE const IConsoleCommand& operator*() const
    {
        return *AsCommand();
    }

    FORCEINLINE IConsoleCommand* operator->()
    {
        return AsCommand();
    }

    FORCEINLINE const IConsoleCommand* operator->() const
    {
        return AsCommand();
    }
};


template<typename T>
class TConsoleVariableData
{
public:
    FORCEINLINE explicit TConsoleVariableData(const T& DefaultValue)
        : Data(DefaultValue)
    { }

    FORCEINLINE T GetValue() const
    {
        // Return by value is intentional
        return Data;
    }

    FORCEINLINE T& operator*()
    {
        return Data;
    }

    FORCEINLINE const T& operator*() const
    {
        return Data;
    }

private:
    T Data;
};


template<typename T>
class TAutoConsoleVariable
    : public FAutoConsoleObject
{
    typedef TConsoleVariableData<T> FConsoleVariableData;

public:
    TAutoConsoleVariable(const CHAR* InName, const T& DefaultValue);

    TAutoConsoleVariable(const CHAR* InName, const T& DefaultValue, const FConsoleVariableDelegate& VariableChangedDelegate)
        : TAutoConsoleVariable(InName, DefaultValue)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FORCEINLINE T GetValue() const
    {
        CHECK(Data != nullptr);
        return Data->GetValue();
    }

    FORCEINLINE IConsoleVariable& operator*()
    {
        return AsVariable();
    }

    FORCEINLINE const IConsoleVariable& operator*() const
    {
        return AsVariable();
    }

    FORCEINLINE IConsoleVariable* operator->()
    {
        return AsVariable();
    }

    FORCEINLINE const IConsoleVariable* operator->() const
    {
        return AsVariable();
    }

private:
    FConsoleVariableData* Data;
};

template<>
FORCEINLINE TAutoConsoleVariable<FString>::TAutoConsoleVariable(const CHAR* InName, const FString& DefaultValue)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, DefaultValue.GetCString()))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetStringData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<int32>::TAutoConsoleVariable(const CHAR* InName, const int32& DefaultValue)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, DefaultValue))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetIntData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<float>::TAutoConsoleVariable(const CHAR* InName, const float& DefaultValue)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, DefaultValue))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetFloatData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<bool>::TAutoConsoleVariable(const CHAR* InName, const bool& bDefaultValue)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, bDefaultValue))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetBoolData());
    CHECK(Data != nullptr);
}
