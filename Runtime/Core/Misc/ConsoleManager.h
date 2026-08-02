#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Templates/Utility/NonCopyable.h"

#ifndef CONSOLE_DEFAULT_HISTORY_LENGTH
    #define CONSOLE_DEFAULT_HISTORY_LENGTH (50)
#endif

DECLARE_DELEGATE(FConsoleCommandDelegate, StringView);
DECLARE_DELEGATE(FConsoleVariableDelegate, struct IConsoleVariable*);

struct IOutputDevice;

template<typename T>
class TConsoleVariableData;

enum class EConsoleVariableFlags : int32
{
    None                  = 0,
    ReadOnly              = FLAG(1), // Variable cannot be changed from the console
    DoNotSetViaConfigFile = FLAG(2), // Variable cannot be set from config files
    SetByConstructor      = FLAG(3), // Indicates that the ConsoleVariable was last set by the default value
    SetByConfigFile       = FLAG(4), // Indicates that the ConsoleVariable was last set by a config-file
    SetByCommandLine      = FLAG(5), // Indicates that the ConsoleVariable was last set by the CommandLine
    SetByConsole          = FLAG(6), // Indicates that the ConsoleVariable was last set by the console
    SetByCode             = FLAG(7), // Indicates that the ConsoleVariable was last set by code
    SetByMask             = SetByConstructor | SetByConfigFile | SetByCommandLine | SetByConsole | SetByCode,
    Default               = None,
};

ENUM_CLASS_OPERATORS(EConsoleVariableFlags);

constexpr const CHAR* SetByFlagToString(EConsoleVariableFlags Flag)
{
    switch (Flag)
    {
        case EConsoleVariableFlags::SetByConstructor: return "Constructor";
        case EConsoleVariableFlags::SetByConfigFile:  return "ConfigFile";
        case EConsoleVariableFlags::SetByCommandLine: return "CommandLine";
        case EConsoleVariableFlags::SetByConsole:     return "Console";
        case EConsoleVariableFlags::SetByCode:        return "Code";
        default:                                      return "Unknown";
    }
}

struct IConsoleObject
{
    virtual ~IConsoleObject() = default;

    /**
     * @brief Cast to a console-variable if the console-variable interface is implemented
     * @return Returns either a console-variable or nullptr
     */
    virtual struct IConsoleVariable* AsVariable() { return nullptr; }

    /**
     * @brief Cast to a console-command if the console-command interface is implemented
     * @return Returns either a console-command or nullptr
     */
    virtual struct IConsoleCommand* AsCommand() { return nullptr; }

    /**
	 * @brief  - Retrieve a short help string describing what the ConsoleObject does
	 * @return - Returns the help string
	 */
    virtual const CHAR* GetHelpString() const = 0;
};

struct IConsoleCommand : public IConsoleObject
{
    /**
     * @brief Execute the ConsoleCommand
     * @param Args Arguments passed to the command
     */
    virtual void Execute(StringView Args) = 0;
};

struct IConsoleVariable : public IConsoleObject
{
    /**
     * @brief Set the variable with an int
     * @param InValue Value to store
     */
    virtual void SetAsInt(int32 InValue, EConsoleVariableFlags Flags) = 0;

    /**
     * @brief Set the variable with an float
     * @param InValue Value to store
     */
    virtual void SetAsFloat(float InValue, EConsoleVariableFlags Flags) = 0;

    /**
     * @brief Set the variable with an bool
     * @param bValue - Value to store
     */
    virtual void SetAsBool(bool bValue, EConsoleVariableFlags Flags) = 0;

    /**
     * @brief Set the variable with an string
     * @param InValue Value to store
     */
    virtual void SetString(const String& InValue, EConsoleVariableFlags Flags) = 0;

    /**
     * @brief Retrieve the variable as an int
     * @return Returns an int with the value of the variable
     */
    virtual int32 GetInt() const = 0;

    /**
     * @brief Retrieve the variable as an float
     * @return Returns an float with the value of the variable
     */
    virtual float GetFloat() const = 0;

    /**
     * @brief Retrieve the variable as an bool
     * @return Returns an bool with the value of the variable
     */
    virtual bool GetBool() const = 0;

    /**
     * @brief Retrieve the variable as an string
     * @return Returns an string with the value of the variable
     */
    virtual String GetString() const = 0;

    /**
     * @brief Retrieve the variable's data as a pointer
     * @return Returns an pointer to int-data if this is a int variable otherwise nullptr
     */
    virtual TConsoleVariableData<int32>* GetIntData() = 0;

    /**
     * @brief Retrieve the variable's datacontainer as a pointer
     * @return Returns an pointer to float-datacontainer if this is a float variable otherwise nullptr
     */
    virtual TConsoleVariableData<float>* GetFloatData() = 0;

    /**
     * @brief Retrieve the variable's datacontainer as a pointer
     * @return Returns an pointer to bool-datacontainer if this is a bool variable otherwise nullptr
     */
    virtual TConsoleVariableData<bool>* GetBoolData() = 0;

    /**
     * @brief Retrieve the variable's datacontainer as a pointer
     * @return Returns an pointer to string-datacontainer if this is a string variable otherwise nullptr
     */
    virtual TConsoleVariableData<String>* GetStringData() = 0;

    /**
    * @brief  - Retrieve the variable's current flags
    * @return - Returns the current flags of the variable
    */
    virtual EConsoleVariableFlags GetFlags() const = 0;

    /**
     * @brief Check weather the variable is an int
     * @return Returns true if the variable is an int
     */
    virtual bool IsVariableInt() const = 0;

    /**
     * @brief Check weather the variable is a float
     * @return Returns true if the variable is a float
     */
    virtual bool IsVariableFloat() const = 0;

    /**
     * @brief Check weather the variable is a bool
     * @return Returns true if the variable is a bool
     */
    virtual bool IsVariableBool() const = 0;

    /**
     * @brief Check weather the variable is a string
     * @return Returns true if the variable is a string
     */
    virtual bool IsVariableString() const = 0;

    /**
     * @brief Set the callback for when the variable changes
     * @param NewChangedDelegate - Delegate for when the variable changes 
     */
    virtual void SetOnChangedDelegate(const FConsoleVariableDelegate& NewChangedDelegate) = 0;

    /**
     * @brief Retrieve the delegate that gets called when the variable changes
     * @return Returns the on changed delegate
     */
    virtual FConsoleVariableDelegate& GetOnChangedDelegate() = 0;

    /**
     * @brief Retrieve the lower bound (clamp range) for an int32 ConsoleVariable
     * @param OutValue Receives the min value when one is set
     * @return Returns true if a min bound is currently set on this variable
     */
    virtual bool TryGetMinValueInt(int32&) const { return false; }

    /**
     * @brief Retrieve the upper bound (clamp range) for an int32 ConsoleVariable
     * @param OutValue Receives the max value when one is set
     * @return Returns true if a max bound is currently set on this variable
     */
    virtual bool TryGetMaxValueInt(int32&) const { return false; }

    /**
     * @brief Set the lower bound (clamp range) for an int32 ConsoleVariable
     *        No-op for non-int variants. Re-clamps the stored value if it falls below the new bound.
     */
    virtual void SetMinValueInt(int32) {}

    /**
     * @brief Set the upper bound (clamp range) for an int32 ConsoleVariable
     *        No-op for non-int variants. Re-clamps the stored value if it exceeds the new bound.
     */
    virtual void SetMaxValueInt(int32) {}

    /**
     * @brief Clear the lower bound for an int32 ConsoleVariable. No-op for non-int variants.
     */
    virtual void ClearMinValueInt() {}

    /**
     * @brief Clear the upper bound for an int32 ConsoleVariable. No-op for non-int variants.
     */
    virtual void ClearMaxValueInt() {}

    /**
     * @brief Retrieve the lower bound (clamp range) for a float ConsoleVariable
     * @param OutValue Receives the min value when one is set
     * @return Returns true if a min bound is currently set on this variable
     */
    virtual bool TryGetMinValueFloat(float&) const { return false; }

    /**
     * @brief Retrieve the upper bound (clamp range) for a float ConsoleVariable
     * @param OutValue Receives the max value when one is set
     * @return Returns true if a max bound is currently set on this variable
     */
    virtual bool TryGetMaxValueFloat(float&) const { return false; }

    /**
     * @brief Set the lower bound (clamp range) for a float ConsoleVariable
     *        No-op for non-float variants. Re-clamps the stored value if it falls below the new bound.
     */
    virtual void SetMinValueFloat(float) {}

    /**
     * @brief Set the upper bound (clamp range) for a float ConsoleVariable
     *        No-op for non-float variants. Re-clamps the stored value if it exceeds the new bound.
     */
    virtual void SetMaxValueFloat(float) {}

    /**
     * @brief Clear the lower bound for a float ConsoleVariable. No-op for non-float variants.
     */
    virtual void ClearMinValueFloat() {}

    /**
     * @brief Clear the upper bound for a float ConsoleVariable. No-op for non-float variants.
     */
    virtual void ClearMaxValueFloat() {}
};


class CORE_API FConsoleManager
{
public:

    /**
     * @brief Retrieve the ConsoleManager instance
     * @return Returns a reference to the ConsoleManager
     */
    static FORCEINLINE FConsoleManager& Get()
    {
        if (!ConsoleManager)
        {
            SafeCreateConsoleManager();
            CHECK(ConsoleManager != nullptr);
        }

        return *ConsoleManager;
    }

    /**
     * @brief Register a new console-command
     * @param Name Name of the console-command
     * @param CommandDelegate CommandDelegate to call when executing the command
     */
    IConsoleCommand* RegisterCommand(const CHAR* InName, const CHAR* HelpString, const FConsoleCommandDelegate& CommandDelegate);

    /**
     * @brief Register a new String ConsoleVariable
     * @param Name Name of the ConsoleVariable
     * @param DefaultValue Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* HelpString, const CHAR* DefaultValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new int32 ConsoleVariable
     * @param Name Name of the ConsoleVariable
     * @param DefaultValue Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* HelpString, int32 DefaultValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new int32 ConsoleVariable with a clamp range
     * @param MinValue Lower bound applied to all writes
     * @param MaxValue Upper bound applied to all writes
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* HelpString, int32 DefaultValue, int32 MinValue, int32 MaxValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new float ConsoleVariable
     * @param Name Name of the ConsoleVariable
     * @param DefaultValue Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* HelpString, float DefaultValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new float ConsoleVariable with a clamp range
     * @param MinValue Lower bound applied to all writes
     * @param MaxValue Upper bound applied to all writes
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* HelpString, float DefaultValue, float MinValue, float MaxValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new bool ConsoleVariable
     * @param Name Name of the ConsoleVariable
     * @param bDefaultValue Default value for the ConsoleVariable
     */
    IConsoleVariable* RegisterVariable(const CHAR* InName, const CHAR* HelpString, bool bDefaultValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new int32 ConsoleVariable that references an externally-owned variable
     * @param InName     Name of the ConsoleVariable
     * @param HelpString Help string describing the variable
     * @param RefValue   Reference to the externally-owned variable (the ConsoleVariable does not own this storage)
     * @param Flags      ConsoleVariable flags
     */
    IConsoleVariable* RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, int32&   RefValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new int32 ConsoleVariable reference with a clamp range
     */
    IConsoleVariable* RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, int32&   RefValue, int32 MinValue, int32 MaxValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new float ConsoleVariable that references an externally-owned variable
     */
    IConsoleVariable* RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, float&   RefValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new float ConsoleVariable reference with a clamp range
     */
    IConsoleVariable* RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, float&   RefValue, float MinValue, float MaxValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new bool ConsoleVariable that references an externally-owned variable
     */
    IConsoleVariable* RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, bool&    RefValue, EConsoleVariableFlags Flags);

    /**
     * @brief Register a new String ConsoleVariable that references an externally-owned variable
     */
    IConsoleVariable* RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, String& RefValue, EConsoleVariableFlags Flags);

    /**
     * @brief Unregister a ConsoleObject
     * @param ConsoleObject ConsoleObject to unregister from the console manager
     */
    void UnregisterObject(IConsoleObject* ConsoleObject);

    /**
     * @brief Check weather or not a console-object exists with a specific name
     * @param Name Name of the console-object
     * @return Returns true if there exists a console-object with the specified name
     */
    bool IsConsoleObject(const CHAR* Name) const;

    /**
     * @brief Finds the name of a ConsoleObject
     * @param ConsoleObject Name of the ConsoleCommand
     * @return The ConsoleCommand matching the name
     */
    String FindConsoleObjectName(IConsoleObject* ConsoleObject);

    /**
     * @brief Finds the ConsoleCommand with the matching name, returns nullptr if not found
     * @param Name Name of the ConsoleCommand
     * @return The ConsoleCommand matching the name
     */
    IConsoleCommand* FindConsoleCommand(const CHAR* Name) const;

    /**
     * @brief Find the ConsoleVariable with the matching name, returns nullptr if not found
     * @param Name Name of the ConsoleVariable
     * @return The ConsoleVariable matching the name
     */
    IConsoleVariable* FindConsoleVariable(const CHAR* Name) const;

    /**
     * @brief Finds a any ConsoleObject with the matching name, returns nullptr if not found
     * @param Name Name of the ConsoleObject
     * @return The ConsoleObject matching the name
     */
    IConsoleObject* FindConsoleObject(const CHAR* Name) const;

    /**
     * @brief Retrieve all ConsoleObjects that fits the name of the specified string
     * @param CandidateName Names to match
     * @param OutCandidates Array to store the console-objects that matches the candidate-name
     */
    void FindCandidates(const StringView& CandidateName, TArray<TPair<IConsoleObject*, String>>& OutCandidates);

    /**
     * @brief Clears the console history
     */
    void ClearHistory();

    /**
     * @brief Execute a string from the console
     * @param OutputDevice OutputDevice to print any messages to
     * @param Command Command to execute by the console
     */
    void ExecuteCommand(IOutputDevice& OutputDevice, const String& Command);

    /**
     * @brief Retrieve all registered console objects
     * @param OutObjects Array to populate with name/object pairs
     */
    void GetConsoleObjects(TArray<TPair<String, IConsoleObject*>>& OutObjects) const;

    /**
     * @brief Dump all console variable values to an output device
     * @param OutputDevice Output device to write to
     * @param Key Optional filter key (case-insensitive substring match)
     */
    void DumpConsoleVariableValues(IOutputDevice& OutputDevice, const CHAR* Key = nullptr);

    /**
     * @brief Apply every matching '-Name[=Value]' command-line option to the already-registered
     *        ConsoleVariables. Required because variables in compile-linked modules register
     *        during static initialization, before the command line has been parsed.
     */
    void LoadConsoleVariablesFromCommandLine();

    /**
     * @brief Retrieve all the history that has been written to the console
     * @return An array containing string of all history written to the console
     */
    const TArray<String>& GetHistory() const
    {
        return History;
    }

private:
    static void SafeCreateConsoleManager();

    FConsoleManager();
    ~FConsoleManager();

    IConsoleObject* RegisterObject(const CHAR* Name, IConsoleObject* Variable);

    int32                          HistoryLength;
    TArray<String>                History;
    TMap<String, IConsoleObject*> ConsoleObjects;

    static FConsoleManager* ConsoleManager;
};


class FAutoConsoleObject : public FNonCopyAndNonMovable
{
public:
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


class FAutoConsoleCommand : public FAutoConsoleObject
{
public:
    FAutoConsoleCommand(const CHAR* InName, const CHAR* InHelpString, const FConsoleCommandDelegate& Delegate)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterCommand(InName, InHelpString, Delegate))
    {
    }

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
    {
    }

    FORCEINLINE T GetValue() const
    {
        return Data; // Return by value is intentional
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
class TAutoConsoleVariable : public FAutoConsoleObject
{
    typedef TConsoleVariableData<T> FConsoleVariableData;

public:
    TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const T& DefaultValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default);

    TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const T& DefaultValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : TAutoConsoleVariable(InName, InHelpString, DefaultValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    /**
     * @brief Construct a value-owning ConsoleVariable with a clamp range. Available only for int32 and float specializations.
     * @param MinValue Lower bound applied to all writes
     * @param MaxValue Upper bound applied to all writes
     */
    TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const T& DefaultValue, const T& MinValue, const T& MaxValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default);

    TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const T& DefaultValue, const T& MinValue, const T& MaxValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : TAutoConsoleVariable(InName, InHelpString, DefaultValue, MinValue, MaxValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    void SetVariable(const T& InValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::SetByCode);
    
    FORCEINLINE T GetValue() const
    {
        CHECK(Data != nullptr);
        return Data->GetValue();
    }

    FORCEINLINE IConsoleVariable& operator*()
    {
        return *AsVariable();
    }

    FORCEINLINE const IConsoleVariable& operator*() const
    {
        return *AsVariable();
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
FORCEINLINE TAutoConsoleVariable<String>::TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const String& DefaultValue, EConsoleVariableFlags InFlags)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, InHelpString, *DefaultValue, InFlags))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetStringData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<int32>::TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const int32& DefaultValue, EConsoleVariableFlags InFlags)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, InHelpString, DefaultValue, InFlags))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetIntData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<float>::TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const float& DefaultValue, EConsoleVariableFlags InFlags)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, InHelpString, DefaultValue, InFlags))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetFloatData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<bool>::TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const bool& bDefaultValue, EConsoleVariableFlags InFlags)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, InHelpString, bDefaultValue, InFlags))
{ 
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetBoolData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<int32>::TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const int32& DefaultValue, const int32& MinValue, const int32& MaxValue, EConsoleVariableFlags InFlags)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, InHelpString, DefaultValue, MinValue, MaxValue, InFlags))
{
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetIntData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE TAutoConsoleVariable<float>::TAutoConsoleVariable(const CHAR* InName, const CHAR* InHelpString, const float& DefaultValue, const float& MinValue, const float& MaxValue, EConsoleVariableFlags InFlags)
    : FAutoConsoleObject(FConsoleManager::Get().RegisterVariable(InName, InHelpString, DefaultValue, MinValue, MaxValue, InFlags))
{
    Data = static_cast<FConsoleVariableData*>(AsVariable()->GetFloatData());
    CHECK(Data != nullptr);
}

template<>
FORCEINLINE void TAutoConsoleVariable<int32>::SetVariable(const int32& InValue, EConsoleVariableFlags InFlags)
{
    AsVariable()->SetAsInt(InValue, InFlags);
}

template<>
FORCEINLINE void TAutoConsoleVariable<float>::SetVariable(const float& InValue, EConsoleVariableFlags InFlags)
{
    AsVariable()->SetAsFloat(InValue, InFlags);
}

template<>
FORCEINLINE void TAutoConsoleVariable<bool>::SetVariable(const bool& bInValue, EConsoleVariableFlags InFlags)
{
    AsVariable()->SetAsBool(bInValue, InFlags);
}

template<>
FORCEINLINE void TAutoConsoleVariable<String>::SetVariable(const String& InValue, EConsoleVariableFlags InFlags)
{
    AsVariable()->SetString(InValue, InFlags);
}


class FAutoConsoleVariableRef : public FAutoConsoleObject
{
public:
    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, int32&   RefValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterVariableRef(InName, InHelpString, RefValue, InFlags))
    {
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, float&   RefValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterVariableRef(InName, InHelpString, RefValue, InFlags))
    {
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, bool&    RefValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterVariableRef(InName, InHelpString, RefValue, InFlags))
    {
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, String& RefValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterVariableRef(InName, InHelpString, RefValue, InFlags))
    {
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, int32&   RefValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleVariableRef(InName, InHelpString, RefValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, float&   RefValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleVariableRef(InName, InHelpString, RefValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, bool&    RefValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleVariableRef(InName, InHelpString, RefValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, String& RefValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleVariableRef(InName, InHelpString, RefValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, int32& RefValue, int32 MinValue, int32 MaxValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterVariableRef(InName, InHelpString, RefValue, MinValue, MaxValue, InFlags))
    {
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, float& RefValue, float MinValue, float MaxValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleObject(FConsoleManager::Get().RegisterVariableRef(InName, InHelpString, RefValue, MinValue, MaxValue, InFlags))
    {
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, int32& RefValue, int32 MinValue, int32 MaxValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleVariableRef(InName, InHelpString, RefValue, MinValue, MaxValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FAutoConsoleVariableRef(const CHAR* InName, const CHAR* InHelpString, float& RefValue, float MinValue, float MaxValue, const FConsoleVariableDelegate& VariableChangedDelegate, EConsoleVariableFlags InFlags = EConsoleVariableFlags::Default)
        : FAutoConsoleVariableRef(InName, InHelpString, RefValue, MinValue, MaxValue, InFlags)
    {
        AsVariable()->SetOnChangedDelegate(VariableChangedDelegate);
    }

    FORCEINLINE void SetVariable(int32 InValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::SetByCode)
    {
        AsVariable()->SetAsInt(InValue, InFlags);
    }

    FORCEINLINE void SetVariable(float InValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::SetByCode)
    {
        AsVariable()->SetAsFloat(InValue, InFlags);
    }

    FORCEINLINE void SetVariable(bool bInValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::SetByCode)
    {
        AsVariable()->SetAsBool(bInValue, InFlags);
    }

    FORCEINLINE void SetVariable(const String& InValue, EConsoleVariableFlags InFlags = EConsoleVariableFlags::SetByCode)
    {
        AsVariable()->SetString(InValue, InFlags);
    }

    FORCEINLINE IConsoleVariable& operator*()
    {
        return *AsVariable();
    }

    FORCEINLINE const IConsoleVariable& operator*() const
    {
        return *AsVariable();
    }

    FORCEINLINE IConsoleVariable* operator->()
    {
        return AsVariable();
    }

    FORCEINLINE const IConsoleVariable* operator->() const
    {
        return AsVariable();
    }
};
