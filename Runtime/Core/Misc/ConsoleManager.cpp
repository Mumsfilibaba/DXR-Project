#include "ConsoleManager.h"
#include "EngineConfig.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Platform/PlatformMisc.h"

FAutoConsoleCommand GClearHistory(
    "ClearHistory",
    "Clears the history of the Console",
    FConsoleCommandDelegate::CreateRaw(&FConsoleManager::Get(), &FConsoleManager::ClearHistory));

TAutoConsoleVariable<FString> GEcho(
    "Echo", 
    "Prints the entered text to the console",
    "",
    FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InVariable) -> void
    {
        if (InVariable->IsVariableString())
        {
            IOutputDevice* OutputDevice = FOutputDeviceLogger::Get();
            OutputDevice->Log(ELogSeverity::Info, InVariable->GetString());
        }
    }));


class FConsoleCommand 
    : public IConsoleCommand
{
public:
    FConsoleCommand(const FConsoleCommandDelegate& Delegate, const CHAR* InHelpString)
        : ExecuteDelegate(Delegate)
        , HelpString(InHelpString)
    { }

    virtual ~FConsoleCommand() = default;

    virtual IConsoleCommand* AsCommand() override final { return this; }
 
    virtual const CHAR* GetHelpString() const override final { return HelpString; }

    // TODO: Add parameters to console commands
    virtual void Execute() override final
    {
        ExecuteDelegate.ExecuteIfBound();
    }

private:
    FConsoleCommandDelegate ExecuteDelegate;
    const CHAR*             HelpString;
};


class FConsoleVariableBase 
    : public IConsoleVariable
{
public:
    FConsoleVariableBase(EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : IConsoleVariable()
        , Flags((InFlags & ~EConsoleVariableFlags::SetByMask) | EConsoleVariableFlags::SetByConstructor)
        , ChangedDelegate()
        , HelpString(InHelpString)
    { }

    virtual ~FConsoleVariableBase() = default;

    virtual IConsoleVariable* AsVariable() override final { return this; }
    
    virtual const CHAR* GetHelpString() const override final { return HelpString; }

    virtual void SetOnChangedDelegate(const FConsoleVariableDelegate& NewChangedDelegate) override final
    {
        ChangedDelegate = NewChangedDelegate;
    }

    virtual FConsoleVariableDelegate& GetOnChangedDelegate() override final
    {
        return ChangedDelegate;
    }

    virtual EConsoleVariableFlags GetFlags() const override final 
    { 
        return Flags; 
    }

protected:
    bool CanBeSet(EConsoleVariableFlags SetBy)
    {
        // Only flags should be sent in here
        CHECK((SetBy & ~EConsoleVariableFlags::SetByMask) == EConsoleVariableFlags::None);

        const bool bIsDefault = (Flags & EConsoleVariableFlags::SetByConstructor) != EConsoleVariableFlags::None;
        if ((Flags & EConsoleVariableFlags::ReadOnly) != EConsoleVariableFlags::None)
        {
            if (!bIsDefault)
            {
                return false;
            }

            if (SetBy == EConsoleVariableFlags::SetByConfigFile)
            {
                return (Flags & EConsoleVariableFlags::DoNotSetViaConfigFile) == EConsoleVariableFlags::None;
            }
            
            return false;
        }
        else if ((Flags & EConsoleVariableFlags::DoNotSetViaConfigFile) != EConsoleVariableFlags::None)
        {
            if (SetBy == EConsoleVariableFlags::SetByConfigFile)
            {
                return false;
            }
        }

        return true;
    }

    void OnChanged(EConsoleVariableFlags SetBy)
    {
        CHECK(CanBeSet(SetBy));

        const EConsoleVariableFlags CurrentSetBy = Flags & EConsoleVariableFlags::SetByMask;
        Flags = (Flags ^ CurrentSetBy) | SetBy;

        ChangedDelegate.ExecuteIfBound(this);
    }

    EConsoleVariableFlags    Flags;
    FConsoleVariableDelegate ChangedDelegate;
    const CHAR*              HelpString;
};


template<typename T>
class TConsoleVariable;

typedef TConsoleVariable<int32>   FConsoleVariableInt32;
typedef TConsoleVariable<float>   FConsoleVariableFloat;
typedef TConsoleVariable<bool>    FConsoleVariableBool;
typedef TConsoleVariable<FString> FConsoleVariableString;

template<typename T>
class TConsoleVariable 
    : public FConsoleVariableBase
{
    using FConsoleVariableBase::OnChanged;

public:
    explicit TConsoleVariable(const T& InDefaultValue, EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : FConsoleVariableBase(InFlags, InHelpString)
        , Data(InDefaultValue)
    { }

    virtual TConsoleVariableData<int32>*   GetIntData()    override final { return nullptr; }
    virtual TConsoleVariableData<float>*   GetFloatData()  override final { return nullptr; }
    virtual TConsoleVariableData<bool>*    GetBoolData()   override final { return nullptr; }
    virtual TConsoleVariableData<FString>* GetStringData() override final { return nullptr; }

    virtual bool IsVariableInt()    const override final { return false; }
    virtual bool IsVariableFloat()  const override final { return false; }
    virtual bool IsVariableBool()   const override final { return false; }
    virtual bool IsVariableString() const override final { return false; }

    virtual void SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)   override final;
    virtual void SetAsFloat(float InValue, EConsoleVariableFlags InFlags) override final;
    virtual void SetAsBool(bool bValue, EConsoleVariableFlags InFlags)    override final;

    virtual void SetString(const FString& InValue, EConsoleVariableFlags InFlags) override final
    {
        if (CanBeSet(InFlags))
        {
            T NewValue = 0;
            if (TTypeFromString<T>::FromString(InValue, NewValue))
            {
                *Data = ::Move(NewValue);
                OnChanged(InFlags);
            }
        }
    }

    virtual int32 GetInt()   const override final;
    virtual float GetFloat() const override final;
    virtual bool  GetBool()  const override final;
    
    virtual FString GetString() const override final
    {
        return TTypeToString<T>::ToString(*Data);
    }

private:
    TConsoleVariableData<T> Data;
};

// Int32

template<>
TConsoleVariableData<int32>* TConsoleVariable<int32>::GetIntData()
{
    return &Data;
}

template<>
bool TConsoleVariable<int32>::IsVariableInt() const
{
    return true;
}

template<>
void TConsoleVariable<int32>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = InValue;
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariable<int32>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = static_cast<int32>(InValue);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariable<int32>::SetAsBool(bool bValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = bValue ? 1 : 0;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariable<int32>::GetInt() const
{
    return *Data;
}

template<>
float TConsoleVariable<int32>::GetFloat() const
{
    return static_cast<float>(*Data);
}

template<>
bool TConsoleVariable<int32>::GetBool() const
{
    return (*Data != 0);
}

// Float

template<>
TConsoleVariableData<float>* TConsoleVariable<float>::GetFloatData()
{
    return &Data;
}

template<>
bool TConsoleVariable<float>::IsVariableFloat() const
{
    return true;
}

template<>
void TConsoleVariable<float>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = static_cast<float>(InValue);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariable<float>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = InValue;
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariable<float>::SetAsBool(bool bValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = bValue ? 1.0f : 0.0f;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariable<float>::GetInt() const
{
    return static_cast<int32>(*Data);
}

template<>
float TConsoleVariable<float>::GetFloat() const
{
    return *Data;
}

template<>
bool TConsoleVariable<float>::GetBool() const
{
    return (*Data != 0.0f);
}

// Bool

template<>
TConsoleVariableData<bool>* TConsoleVariable<bool>::GetBoolData()
{
    return &Data;
}

template<>
bool TConsoleVariable<bool>::IsVariableBool() const
{
    return true;
}

template<>
void TConsoleVariable<bool>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = (InValue != 0);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariable<bool>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = (InValue != 0.0f);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariable<bool>::SetAsBool(bool bValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = bValue;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariable<bool>::GetInt() const
{
    return *Data ? 1 : 0;
}

template<>
float TConsoleVariable<bool>::GetFloat() const
{
    return *Data ? 1.0f : 0.0f;
}

template<>
bool TConsoleVariable<bool>::GetBool() const
{
    return *Data;
}

// FString

template<>
TConsoleVariableData<FString>* TConsoleVariable<FString>::GetStringData()
{
    return &Data;
}

template<>
bool TConsoleVariable<FString>::IsVariableString() const
{
    return true;
}

template<>
inline void TConsoleVariable<FString>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = TTypeToString<int32>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariable<FString>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = TTypeToString<float>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariable<FString>::SetAsBool(bool InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = TTypeToString<bool>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariable<FString>::SetString(const FString& InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = InValue;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariable<FString>::GetInt() const
{
    int32 Value = false;
    TTypeFromString<int32>::FromString(Data.GetValue(), Value);
    return Value;
}

template<>
float TConsoleVariable<FString>::GetFloat() const
{
    float Value = false;
    TTypeFromString<float>::FromString(Data.GetValue(), Value);
    return Value;
}

template<>
bool TConsoleVariable<FString>::GetBool() const
{
    bool bValue = false;
    TTypeFromString<bool>::FromString(Data.GetValue(), bValue);
    return bValue;
}

template<>
FString TConsoleVariable<FString>::GetString() const
{
    return Data.GetValue();
}


FConsoleManager* FConsoleManager::GInstance;

void FConsoleManager::CreateConsoleManager()
{
    CHECK(GInstance == nullptr);

    if (!GInstance)
    {
        GInstance = new FConsoleManager();
    }

    CHECK(GInstance != nullptr);
}

FConsoleManager::~FConsoleManager()
{
    for (auto ConsoleObject : ConsoleObjects)
    {
        IConsoleObject* Object = ConsoleObject.second;
        delete Object;
    }

    ConsoleObjects.clear();
}

IConsoleCommand* FConsoleManager::RegisterCommand(const CHAR* InName, const CHAR* HelpString, const FConsoleCommandDelegate& CommandDelegate)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleCommand(CommandDelegate, HelpString)))
    {
        return NewObject->AsCommand();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, const CHAR* DefaultValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableString(DefaultValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, int32 DefaultValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableInt32(DefaultValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, float DefaultValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableFloat(DefaultValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, bool bDefaultValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableBool(bDefaultValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

void FConsoleManager::UnregisterObject(IConsoleObject* ConsoleObject)
{
    const FString Name = FindConsoleObjectName(ConsoleObject);

    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject != ConsoleObjects.end())
    {
        // Delete and erase reference to object
        IConsoleObject* Object = ExistingObject->second;
        delete Object;

        ConsoleObjects.erase(ExistingObject);
    }
}

bool FConsoleManager::IsConsoleObject(const CHAR* InName) const
{
    return (FindConsoleObject(InName) != nullptr);
}

FString FConsoleManager::FindConsoleObjectName(IConsoleObject* ConsoleObject)
{
    for (const auto CurrentObject : ConsoleObjects)
    {
        if (ConsoleObject == CurrentObject.second)
        {
            return CurrentObject.first;
        }
    }

    return FString();
}

IConsoleCommand* FConsoleManager::FindConsoleCommand(const CHAR* Name) const
{
    if (IConsoleObject* Object = FindConsoleObject(Name))
    {
        return Object->AsCommand();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::FindConsoleVariable(const CHAR* Name) const
{
    if (IConsoleObject* Object = FindConsoleObject(Name))
    {
        return Object->AsVariable();
    }

    return nullptr;
}

IConsoleObject* FConsoleManager::FindConsoleObject(const CHAR* InName) const
{
    const FString Name(InName);

    auto ExisitingObject = ConsoleObjects.find(Name);
    if (ExisitingObject != ConsoleObjects.end())
    {
        return ExisitingObject->second;
    }
    else
    {
        return nullptr;
    }
}

void FConsoleManager::ClearHistory()
{
    History.Clear();
}

void FConsoleManager::FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates)
{
    for (const auto& Object : ConsoleObjects)
    {
        const FString& ObjectName = Object.first;

        int32 Length = CandidateName.GetLength();
        if (Length <= ObjectName.GetLength())
        {
            const CHAR* Command = ObjectName.GetCString();
            const CHAR* WordIt  = CandidateName.GetCString();

            int32 CharDiff = -1;
            while (Length > 0 && (CharDiff = (toupper(*WordIt) - toupper(*Command))) == 0)
            {
                Command++;
                WordIt++;
                Length--;
            }

            if (CharDiff == 0)
            {
                IConsoleObject* ConsoleObject = Object.second;
                OutCandidates.Emplace(ConsoleObject, ObjectName);
            }
        }
    }
}

void FConsoleManager::ExecuteCommand(IOutputDevice& OutputDevice, const FString& Command)
{
    OutputDevice.Log(ELogSeverity::Info, Command);

    // Erase history
    History.Emplace(Command);
    if (History.Size() > HistoryLength)
    {
        History.RemoveAt(History.StartIterator());
    }

    int32 Pos = Command.FindChar(' ');
    if (Pos == FString::INVALID_INDEX)
    {
        IConsoleCommand* CommandObject = FindConsoleCommand(Command.GetCString());
        if (!CommandObject)
        {
            OutputDevice.Log(ELogSeverity::Error, "'" + Command + "' is not a registered command");
        }
        else
        {
            CommandObject->Execute();
        }
    }
    else
    {
        const FString VariableName(Command.GetCString(), Pos);

        IConsoleVariable* VariableObject = FindConsoleVariable(VariableName.GetCString());
        if (!VariableObject)
        {
            OutputDevice.Log(ELogSeverity::Error, "'" + Command + "' is not a registered variable");
            return;
        }

        Pos++;

        const FString Value(Command.GetCString() + Pos, Command.GetLength() - Pos);
        if (TTryParseType<int64>::TryParse(Value))
        {
            VariableObject->SetString(Value, EConsoleVariableFlags::SetByConsole);
        }
        else if (TTryParseType<float>::TryParse(Value) && VariableObject->IsVariableFloat())
        {
            VariableObject->SetString(Value, EConsoleVariableFlags::SetByConsole);
        }
        else if (TTryParseType<bool>::TryParse(Value) && VariableObject->IsVariableBool())
        {
            VariableObject->SetString(Value, EConsoleVariableFlags::SetByConsole);
        }
        else
        {
            if (VariableObject->IsVariableString())
            {
                VariableObject->SetString(Value, EConsoleVariableFlags::SetByConsole);
            }
            else
            {
                OutputDevice.Log(ELogSeverity::Error, "'" + Value + "' Is an invalid value for '" + VariableName + "'");
            }
        }
    }
}

IConsoleObject* FConsoleManager::RegisterObject(const CHAR* InName, IConsoleObject* Object)
{
    const FString Name(InName);

    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject != ConsoleObjects.end())
    {
        LOG_WARNING("Trying to register an already existing ConsoleObject '%s'", InName);
        return ExistingObject->second;
    }

    auto Result = ConsoleObjects.insert(std::make_pair(Name, Object));
    if (Result.second)
    {
        // TODO: Refactor this, right now it only works with a single ConfigFile
        if (IConsoleVariable* Variable = Object->AsVariable())
        {
            FStringView CommandLineValue;
            if (FCommandLine::Parse(InName, CommandLineValue))
            {
                const FString Value = FString(CommandLineValue);
                Variable->SetString(Value, EConsoleVariableFlags::SetByCommandLine);
            }
            else if (GConfig)
            {
                FString Value;
                if (GConfig->GetString("", InName, Value))
                {
                    Variable->SetString(Value, EConsoleVariableFlags::SetByConfigFile);
                }
            }
        }

        LOG_INFO("Registered ConsoleObject '%s'", Name.GetCString());
        return Result.first->second;
    }

    LOG_ERROR("Failed to register ConsoleObject '%s'", InName);
    return nullptr;
}
