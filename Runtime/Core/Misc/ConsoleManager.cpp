#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Config.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/FileOutputDevice.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Templates/TypeTraits/EqualTraits.h"

static FAutoConsoleCommand CCmdClearHistory(
    "ClearHistory",
    "Clears the history of the Console",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        FConsoleManager::Get().ClearHistory();
    }));

static FAutoConsoleCommand CCmdDumpCVars(
    "Console.DumpCVars",
    "Dumps all console variables (optionally filtered by key) to DumpConsoleVariableValues.txt in the current working directory",
    FConsoleCommandDelegate::CreateLambda([](StringView Args)
    {
        FConsoleManager& ConsoleManager = FConsoleManager::Get();

        const String FilePath = FPlatformFile::GetCurrentWorkingDirectory() + "/DumpConsoleVariableValues.txt";
        FFileOutputDevice OutputDevice(FilePath);

        StringView KeyView = Args;
        KeyView.TrimInline();
        
        const CHAR* Key = KeyView.IsEmpty() ? nullptr : *KeyView;
        ConsoleManager.DumpConsoleVariableValues(OutputDevice, Key);
    }));

static TAutoConsoleVariable<String> CVarEcho(
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

template<typename T>
class TConsoleVariable;

template<typename T>
class TConsoleVariableRef;

template<typename T>
class TBoundedConsoleVariable;

template<typename T>
class TBoundedConsoleVariableRef;

typedef TConsoleVariable<bool>         FConsoleVariableBool;
typedef TConsoleVariable<String>       FConsoleVariableString;
typedef TBoundedConsoleVariable<int32> FConsoleVariableInt32;
typedef TBoundedConsoleVariable<float> FConsoleVariableFloat;

typedef TConsoleVariableRef<bool>         FConsoleVariableBoolRef;
typedef TConsoleVariableRef<String>       FConsoleVariableStringRef;
typedef TBoundedConsoleVariableRef<int32> FConsoleVariableInt32Ref;
typedef TBoundedConsoleVariableRef<float> FConsoleVariableFloatRef;


class FConsoleCommand  : public IConsoleCommand
{
public:
    FConsoleCommand(const FConsoleCommandDelegate& Delegate, const CHAR* InHelpString)
        : ExecuteDelegate(Delegate)
        , HelpString(InHelpString)
    {
    }

    virtual ~FConsoleCommand() = default;

    virtual IConsoleCommand* AsCommand() override final
    {
        return this;
    }
 
    virtual const CHAR* GetHelpString() const override final
    {
        return HelpString;
    }

    virtual void Execute(StringView Args) override final
    {
        ExecuteDelegate.ExecuteIfBound(Args);
    }

private:
    FConsoleCommandDelegate ExecuteDelegate;
    const CHAR*             HelpString;
};


class FConsoleVariableBase  : public IConsoleVariable
{
public:
    FConsoleVariableBase(EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : IConsoleVariable()
        , Flags((InFlags & ~EConsoleVariableFlags::SetByMask) | EConsoleVariableFlags::SetByConstructor)
        , ChangedDelegate()
        , HelpString(InHelpString)
    {
    }

    virtual ~FConsoleVariableBase() = default;

    virtual IConsoleVariable* AsVariable() override final
    {
        return this;
    }
    
    virtual const CHAR* GetHelpString() const override final
    {
        return HelpString;
    }

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
class TConsoleVariable  : public FConsoleVariableBase
{
protected:
    using FConsoleVariableBase::OnChanged;

public:
    explicit TConsoleVariable(const T& InDefaultValue, EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : FConsoleVariableBase(InFlags, InHelpString)
        , Data(InDefaultValue)
    {
    }

    virtual TConsoleVariableData<int32>*  GetIntData()    override final { return nullptr; }
    virtual TConsoleVariableData<float>*  GetFloatData()  override final { return nullptr; }
    virtual TConsoleVariableData<bool>*   GetBoolData()   override final { return nullptr; }
    virtual TConsoleVariableData<String>* GetStringData() override final { return nullptr; }

    virtual bool IsVariableInt()    const override final { return false; }
    virtual bool IsVariableFloat()  const override final { return false; }
    virtual bool IsVariableBool()   const override final { return false; }
    virtual bool IsVariableString() const override final { return false; }

    virtual void SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)   override;
    virtual void SetAsFloat(float InValue, EConsoleVariableFlags InFlags) override;
    virtual void SetAsBool(bool bValue, EConsoleVariableFlags InFlags)    override;

    virtual void SetString(const String& InValue, EConsoleVariableFlags InFlags) override
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

    virtual String GetString() const override final
    {
        return TTypeToString<T>::ToString(*Data);
    }

protected:
    TConsoleVariableData<T> Data;
};

// -------------------------------------------------------------------------------------------
// Int32
// -------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------
// Float
// -------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------
// Bool
// -------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------
// String
// -------------------------------------------------------------------------------------------

template<>
TConsoleVariableData<String>* TConsoleVariable<String>::GetStringData()
{
    return &Data;
}

template<>
bool TConsoleVariable<String>::IsVariableString() const
{
    return true;
}

template<>
inline void TConsoleVariable<String>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = TTypeToString<int32>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariable<String>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = TTypeToString<float>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariable<String>::SetAsBool(bool InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = TTypeToString<bool>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariable<String>::SetString(const String& InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *Data = InValue;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariable<String>::GetInt() const
{
    int32 Value = false;
    TTypeFromString<int32>::FromString(Data.GetValue(), Value);
    return Value;
}

template<>
float TConsoleVariable<String>::GetFloat() const
{
    float Value = false;
    TTypeFromString<float>::FromString(Data.GetValue(), Value);
    return Value;
}

template<>
bool TConsoleVariable<String>::GetBool() const
{
    bool bValue = false;
    TTypeFromString<bool>::FromString(Data.GetValue(), bValue);
    return bValue;
}

template<>
String TConsoleVariable<String>::GetString() const
{
    return Data.GetValue();
}


template<typename T>
class TBoundedConsoleVariable : public TConsoleVariable<T>
{
public:
    explicit TBoundedConsoleVariable(const T& InDefaultValue, EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : TConsoleVariable<T>(InDefaultValue, InFlags, InHelpString)
        , MinValue(T{})
        , MaxValue(T{})
        , bHasMin(false)
        , bHasMax(false)
    {
    }

    virtual void SetAsInt(int32 InValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            *this->Data = ClampToRange(static_cast<T>(InValue));
            this->OnChanged(InFlags);
        }
    }

    virtual void SetAsFloat(float InValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            *this->Data = ClampToRange(static_cast<T>(InValue));
            this->OnChanged(InFlags);
        }
    }

    virtual void SetAsBool(bool bValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            *this->Data = ClampToRange(static_cast<T>(bValue ? 1 : 0));
            this->OnChanged(InFlags);
        }
    }

    virtual void SetString(const String& InValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            T NewValue = 0;
            if (TTypeFromString<T>::FromString(InValue, NewValue))
            {
                *this->Data = ClampToRange(::Move(NewValue));
                this->OnChanged(InFlags);
            }
        }
    }

    virtual bool TryGetMinValueInt(int32& OutValue)   const override final;
    virtual bool TryGetMaxValueInt(int32& OutValue)   const override final;
    virtual void SetMinValueInt(int32 InValue)              override final;
    virtual void SetMaxValueInt(int32 InValue)              override final;
    virtual void ClearMinValueInt()                         override final;
    virtual void ClearMaxValueInt()                         override final;

    virtual bool TryGetMinValueFloat(float& OutValue) const override final;
    virtual bool TryGetMaxValueFloat(float& OutValue) const override final;
    virtual void SetMinValueFloat(float InValue)            override final;
    virtual void SetMaxValueFloat(float InValue)            override final;
    virtual void ClearMinValueFloat()                       override final;
    virtual void ClearMaxValueFloat()                       override final;

private:

    // Clamps to [MinValue, MaxValue] when either bound is set; pass-through otherwise.
    T ClampToRange(T InValue) const
    {
        if (bHasMin && InValue < MinValue)
        {
            return MinValue;
        }

        if (bHasMax && InValue > MaxValue)
        {
            return MaxValue;
        }

        return InValue;
    }

    T    MinValue;
    T    MaxValue;
    bool bHasMin;
    bool bHasMax;
};

template<typename T>
bool TBoundedConsoleVariable<T>::TryGetMinValueInt(int32& OutValue) const
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        if (bHasMin)
        {
            OutValue = MinValue;
            return true;
        }
    }

    return false;
}

template<typename T>
bool TBoundedConsoleVariable<T>::TryGetMaxValueInt(int32& OutValue) const
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        if (bHasMax)
        {
            OutValue = MaxValue;
            return true;
        }
    }

    return false;
}

template<typename T>
void TBoundedConsoleVariable<T>::SetMinValueInt(int32 InValue)
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        MinValue = InValue;
        bHasMin  = true;

        if (bHasMax && MaxValue < MinValue)
        {
            MaxValue = MinValue;
        }

        *this->Data = ClampToRange(*this->Data);
    }
}

template<typename T>
void TBoundedConsoleVariable<T>::SetMaxValueInt(int32 InValue)
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        MaxValue = InValue;
        bHasMax  = true;

        if (bHasMin && MinValue > MaxValue)
        {
            MinValue = MaxValue;
        }

        *this->Data = ClampToRange(*this->Data);
    }
}

template<typename T>
void TBoundedConsoleVariable<T>::ClearMinValueInt()
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        bHasMin  = false;
        MinValue = T{};
    }
}

template<typename T>
void TBoundedConsoleVariable<T>::ClearMaxValueInt()
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        bHasMax  = false;
        MaxValue = T{};
    }
}

template<typename T>
bool TBoundedConsoleVariable<T>::TryGetMinValueFloat(float& OutValue) const
{
    if constexpr (TIsSame<T, float>::Value)
    {
        if (bHasMin)
        {
            OutValue = MinValue;
            return true;
        }
    }

    return false;
}

template<typename T>
bool TBoundedConsoleVariable<T>::TryGetMaxValueFloat(float& OutValue) const
{
    if constexpr (TIsSame<T, float>::Value)
    {
        if (bHasMax)
        {
            OutValue = MaxValue;
            return true;
        }
    }

    return false;
}

template<typename T>
void TBoundedConsoleVariable<T>::SetMinValueFloat(float InValue)
{
    if constexpr (TIsSame<T, float>::Value)
    {
        MinValue = InValue;
        bHasMin  = true;

        if (bHasMax && MaxValue < MinValue)
        {
            MaxValue = MinValue;
        }

        *this->Data = ClampToRange(*this->Data);
    }
}

template<typename T>
void TBoundedConsoleVariable<T>::SetMaxValueFloat(float InValue)
{
    if constexpr (TIsSame<T, float>::Value)
    {
        MaxValue = InValue;
        bHasMax  = true;

        if (bHasMin && MinValue > MaxValue)
        {
            MinValue = MaxValue;
        }

        *this->Data = ClampToRange(*this->Data);
    }
}

template<typename T>
void TBoundedConsoleVariable<T>::ClearMinValueFloat()
{
    if constexpr (TIsSame<T, float>::Value)
    {
        bHasMin  = false;
        MinValue = T{};
    }
}

template<typename T>
void TBoundedConsoleVariable<T>::ClearMaxValueFloat()
{
    if constexpr (TIsSame<T, float>::Value)
    {
        bHasMax  = false;
        MaxValue = T{};
    }
}


template<typename T>
class TConsoleVariableRef : public FConsoleVariableBase
{
protected:
    using FConsoleVariableBase::OnChanged;

public:
    explicit TConsoleVariableRef(T& InRef, EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : FConsoleVariableBase(InFlags, InHelpString)
        , DataPtr(&InRef)
    {
    }

    virtual TConsoleVariableData<int32>*   GetIntData()    override final { return nullptr; }
    virtual TConsoleVariableData<float>*   GetFloatData()  override final { return nullptr; }
    virtual TConsoleVariableData<bool>*    GetBoolData()   override final { return nullptr; }
    virtual TConsoleVariableData<String>* GetStringData() override final { return nullptr; }

    virtual bool IsVariableInt()    const override final { return false; }
    virtual bool IsVariableFloat()  const override final { return false; }
    virtual bool IsVariableBool()   const override final { return false; }
    virtual bool IsVariableString() const override final { return false; }

    virtual void SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)   override;
    virtual void SetAsFloat(float InValue, EConsoleVariableFlags InFlags) override;
    virtual void SetAsBool(bool bValue, EConsoleVariableFlags InFlags)    override;

    virtual void SetString(const String& InValue, EConsoleVariableFlags InFlags) override
    {
        if (CanBeSet(InFlags))
        {
            T NewValue = 0;
            if (TTypeFromString<T>::FromString(InValue, NewValue))
            {
                *DataPtr = ::Move(NewValue);
                OnChanged(InFlags);
            }
        }
    }

    virtual int32 GetInt()   const override final;
    virtual float GetFloat() const override final;
    virtual bool  GetBool()  const override final;

    virtual String GetString() const override final
    {
        return TTypeToString<T>::ToString(*DataPtr);
    }

protected:
    T* DataPtr;
};

// -------------------------------------------------------------------------------------------
// Int32 ref
// -------------------------------------------------------------------------------------------

template<>
bool TConsoleVariableRef<int32>::IsVariableInt() const
{
    return true;
}

template<>
void TConsoleVariableRef<int32>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = InValue;
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariableRef<int32>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = static_cast<int32>(InValue);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariableRef<int32>::SetAsBool(bool bValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = bValue ? 1 : 0;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariableRef<int32>::GetInt() const
{
    return *DataPtr;
}

template<>
float TConsoleVariableRef<int32>::GetFloat() const
{
    return static_cast<float>(*DataPtr);
}

template<>
bool TConsoleVariableRef<int32>::GetBool() const
{
    return (*DataPtr != 0);
}

// -------------------------------------------------------------------------------------------
// Float ref
// -------------------------------------------------------------------------------------------

template<>
bool TConsoleVariableRef<float>::IsVariableFloat() const
{
    return true;
}

template<>
void TConsoleVariableRef<float>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = static_cast<float>(InValue);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariableRef<float>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = InValue;
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariableRef<float>::SetAsBool(bool bValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = bValue ? 1.0f : 0.0f;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariableRef<float>::GetInt() const
{
    return static_cast<int32>(*DataPtr);
}

template<>
float TConsoleVariableRef<float>::GetFloat() const
{
    return *DataPtr;
}

template<>
bool TConsoleVariableRef<float>::GetBool() const
{
    return (*DataPtr != 0.0f);
}

// -------------------------------------------------------------------------------------------
// Bool ref
// -------------------------------------------------------------------------------------------

template<>
bool TConsoleVariableRef<bool>::IsVariableBool() const
{
    return true;
}

template<>
void TConsoleVariableRef<bool>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = (InValue != 0);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariableRef<bool>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = (InValue != 0.0f);
        OnChanged(InFlags);
    }
}

template<>
void TConsoleVariableRef<bool>::SetAsBool(bool bValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = bValue;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariableRef<bool>::GetInt() const
{
    return *DataPtr ? 1 : 0;
}

template<>
float TConsoleVariableRef<bool>::GetFloat() const
{
    return *DataPtr ? 1.0f : 0.0f;
}

template<>
bool TConsoleVariableRef<bool>::GetBool() const
{
    return *DataPtr;
}

// -------------------------------------------------------------------------------------------
// String ref
// -------------------------------------------------------------------------------------------

template<>
bool TConsoleVariableRef<String>::IsVariableString() const
{
    return true;
}

template<>
inline void TConsoleVariableRef<String>::SetAsInt(int32 InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = TTypeToString<int32>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariableRef<String>::SetAsFloat(float InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = TTypeToString<float>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariableRef<String>::SetAsBool(bool InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = TTypeToString<bool>::ToString(InValue);
        OnChanged(InFlags);
    }
}

template<>
inline void TConsoleVariableRef<String>::SetString(const String& InValue, EConsoleVariableFlags InFlags)
{
    if (CanBeSet(InFlags))
    {
        *DataPtr = InValue;
        OnChanged(InFlags);
    }
}

template<>
int32 TConsoleVariableRef<String>::GetInt() const
{
    int32 Value = 0;
    TTypeFromString<int32>::FromString(*DataPtr, Value);
    return Value;
}

template<>
float TConsoleVariableRef<String>::GetFloat() const
{
    float Value = 0.0f;
    TTypeFromString<float>::FromString(*DataPtr, Value);
    return Value;
}

template<>
bool TConsoleVariableRef<String>::GetBool() const
{
    bool bValue = false;
    TTypeFromString<bool>::FromString(*DataPtr, bValue);
    return bValue;
}

template<>
String TConsoleVariableRef<String>::GetString() const
{
    return *DataPtr;
}


template<typename T>
class TBoundedConsoleVariableRef : public TConsoleVariableRef<T>
{
public:
    explicit TBoundedConsoleVariableRef(T& InRef, EConsoleVariableFlags InFlags, const CHAR* InHelpString)
        : TConsoleVariableRef<T>(InRef, InFlags, InHelpString)
        , MinValue(T{})
        , MaxValue(T{})
        , bHasMin(false)
        , bHasMax(false)
    {
    }

    virtual void SetAsInt(int32 InValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            *this->DataPtr = ClampToRange(static_cast<T>(InValue));
            this->OnChanged(InFlags);
        }
    }

    virtual void SetAsFloat(float InValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            *this->DataPtr = ClampToRange(static_cast<T>(InValue));
            this->OnChanged(InFlags);
        }
    }

    virtual void SetAsBool(bool bValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            *this->DataPtr = ClampToRange(static_cast<T>(bValue ? 1 : 0));
            this->OnChanged(InFlags);
        }
    }

    virtual void SetString(const String& InValue, EConsoleVariableFlags InFlags) override final
    {
        if (this->CanBeSet(InFlags))
        {
            T NewValue = 0;
            if (TTypeFromString<T>::FromString(InValue, NewValue))
            {
                *this->DataPtr = ClampToRange(::Move(NewValue));
                this->OnChanged(InFlags);
            }
        }
    }

    virtual bool TryGetMinValueInt(int32& OutValue)   const override final;
    virtual bool TryGetMaxValueInt(int32& OutValue)   const override final;
    virtual void SetMinValueInt(int32 InValue)              override final;
    virtual void SetMaxValueInt(int32 InValue)              override final;
    virtual void ClearMinValueInt()                         override final;
    virtual void ClearMaxValueInt()                         override final;

    virtual bool TryGetMinValueFloat(float& OutValue) const override final;
    virtual bool TryGetMaxValueFloat(float& OutValue) const override final;
    virtual void SetMinValueFloat(float InValue)            override final;
    virtual void SetMaxValueFloat(float InValue)            override final;
    virtual void ClearMinValueFloat()                       override final;
    virtual void ClearMaxValueFloat()                       override final;

private:

    // Clamps to [MinValue, MaxValue] when either bound is set; pass-through otherwise.
    T ClampToRange(T InValue) const
    {
        if (bHasMin && InValue < MinValue)
        {
            return MinValue;
        }

        if (bHasMax && InValue > MaxValue)
        {
            return MaxValue;
        }

        return InValue;
    }

    T    MinValue;
    T    MaxValue;
    bool bHasMin;
    bool bHasMax;
};

template<typename T>
bool TBoundedConsoleVariableRef<T>::TryGetMinValueInt(int32& OutValue) const
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        if (bHasMin)
        {
            OutValue = MinValue;
            return true;
        }
    }

    return false;
}

template<typename T>
bool TBoundedConsoleVariableRef<T>::TryGetMaxValueInt(int32& OutValue) const
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        if (bHasMax)
        {
            OutValue = MaxValue;
            return true;
        }
    }

    return false;
}

template<typename T>
void TBoundedConsoleVariableRef<T>::SetMinValueInt(int32 InValue)
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        MinValue = InValue;
        bHasMin  = true;

        if (bHasMax && MaxValue < MinValue)
        {
            MaxValue = MinValue;
        }

        *this->DataPtr = ClampToRange(*this->DataPtr);
    }
}

template<typename T>
void TBoundedConsoleVariableRef<T>::SetMaxValueInt(int32 InValue)
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        MaxValue = InValue;
        bHasMax  = true;

        if (bHasMin && MinValue > MaxValue)
        {
            MinValue = MaxValue;
        }

        *this->DataPtr = ClampToRange(*this->DataPtr);
    }
}

template<typename T>
void TBoundedConsoleVariableRef<T>::ClearMinValueInt()
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        bHasMin  = false;
        MinValue = T{};
    }
}

template<typename T>
void TBoundedConsoleVariableRef<T>::ClearMaxValueInt()
{
    if constexpr (TIsSame<T, int32>::Value)
    {
        bHasMax  = false;
        MaxValue = T{};
    }
}

template<typename T>
bool TBoundedConsoleVariableRef<T>::TryGetMinValueFloat(float& OutValue) const
{
    if constexpr (TIsSame<T, float>::Value)
    {
        if (bHasMin)
        {
            OutValue = MinValue;
            return true;
        }
    }

    return false;
}

template<typename T>
bool TBoundedConsoleVariableRef<T>::TryGetMaxValueFloat(float& OutValue) const
{
    if constexpr (TIsSame<T, float>::Value)
    {
        if (bHasMax)
        {
            OutValue = MaxValue;
            return true;
        }
    }

    return false;
}

template<typename T>
void TBoundedConsoleVariableRef<T>::SetMinValueFloat(float InValue)
{
    if constexpr (TIsSame<T, float>::Value)
    {
        MinValue = InValue;
        bHasMin  = true;

        if (bHasMax && MaxValue < MinValue)
        {
            MaxValue = MinValue;
        }

        *this->DataPtr = ClampToRange(*this->DataPtr);
    }
}

template<typename T>
void TBoundedConsoleVariableRef<T>::SetMaxValueFloat(float InValue)
{
    if constexpr (TIsSame<T, float>::Value)
    {
        MaxValue = InValue;
        bHasMax  = true;

        if (bHasMin && MinValue > MaxValue)
        {
            MinValue = MaxValue;
        }

        *this->DataPtr = ClampToRange(*this->DataPtr);
    }
}

template<typename T>
void TBoundedConsoleVariableRef<T>::ClearMinValueFloat()
{
    if constexpr (TIsSame<T, float>::Value)
    {
        bHasMin  = false;
        MinValue = T{};
    }
}

template<typename T>
void TBoundedConsoleVariableRef<T>::ClearMaxValueFloat()
{
    if constexpr (TIsSame<T, float>::Value)
    {
        bHasMax  = false;
        MaxValue = T{};
    }
}


FConsoleManager* FConsoleManager::ConsoleManager;

void FConsoleManager::SafeCreateConsoleManager()
{
    CHECK(ConsoleManager == nullptr);

    if (!ConsoleManager)
    {
        ConsoleManager = new FConsoleManager();
    }

    CHECK(ConsoleManager != nullptr);
}

FConsoleManager::FConsoleManager()
    : HistoryLength(CONSOLE_DEFAULT_HISTORY_LENGTH)
    , History()
    , ConsoleObjects()
{
}

FConsoleManager::~FConsoleManager()
{
    for (auto ConsoleObject : ConsoleObjects)
    {
        SAFE_DELETE(ConsoleObject.Second);
    }

    ConsoleObjects.Clear();
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

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, int32 DefaultValue, int32 MinValue, int32 MaxValue, EConsoleVariableFlags Flags)
{
    IConsoleVariable* NewVariable = RegisterVariable(InName, HelpString, DefaultValue, Flags);
    if (NewVariable)
    {
        NewVariable->SetMinValueInt(MinValue);
        NewVariable->SetMaxValueInt(MaxValue);
    }

    return NewVariable;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, float DefaultValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableFloat(DefaultValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, float DefaultValue, float MinValue, float MaxValue, EConsoleVariableFlags Flags)
{
    IConsoleVariable* NewVariable = RegisterVariable(InName, HelpString, DefaultValue, Flags);
    if (NewVariable)
    {
        NewVariable->SetMinValueFloat(MinValue);
        NewVariable->SetMaxValueFloat(MaxValue);
    }

    return NewVariable;
}

IConsoleVariable* FConsoleManager::RegisterVariable(const CHAR* InName, const CHAR* HelpString, bool bDefaultValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableBool(bDefaultValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, int32& RefValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableInt32Ref(RefValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, int32& RefValue, int32 MinValue, int32 MaxValue, EConsoleVariableFlags Flags)
{
    IConsoleVariable* NewVariable = RegisterVariableRef(InName, HelpString, RefValue, Flags);
    if (NewVariable)
    {
        NewVariable->SetMinValueInt(MinValue);
        NewVariable->SetMaxValueInt(MaxValue);
    }

    return NewVariable;
}

IConsoleVariable* FConsoleManager::RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, float& RefValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableFloatRef(RefValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, float& RefValue, float MinValue, float MaxValue, EConsoleVariableFlags Flags)
{
    IConsoleVariable* NewVariable = RegisterVariableRef(InName, HelpString, RefValue, Flags);
    if (NewVariable)
    {
        NewVariable->SetMinValueFloat(MinValue);
        NewVariable->SetMaxValueFloat(MaxValue);
    }

    return NewVariable;
}

IConsoleVariable* FConsoleManager::RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, bool& RefValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableBoolRef(RefValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

IConsoleVariable* FConsoleManager::RegisterVariableRef(const CHAR* InName, const CHAR* HelpString, String& RefValue, EConsoleVariableFlags Flags)
{
    if (IConsoleObject* NewObject = RegisterObject(InName, new FConsoleVariableStringRef(RefValue, Flags, HelpString)))
    {
        return NewObject->AsVariable();
    }

    return nullptr;
}

void FConsoleManager::UnregisterObject(IConsoleObject* ConsoleObject)
{
    const String Name = FindConsoleObjectName(ConsoleObject);
    if (IConsoleObject** Object = ConsoleObjects.Find(Name))
    {
        // Delete and erase reference to object
        delete *Object;
        ConsoleObjects.Remove(Name);
    }
}

bool FConsoleManager::IsConsoleObject(const CHAR* InName) const
{
    return FindConsoleObject(InName) != nullptr;
}

String FConsoleManager::FindConsoleObjectName(IConsoleObject* ConsoleObject)
{
    for (auto CurrentObject : ConsoleObjects)
    {
        if (ConsoleObject == CurrentObject.Second)
        {
            return CurrentObject.First;
        }
    }

    return String();
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
    const String Name(InName);
    if (IConsoleObject* const* Object = ConsoleObjects.Find(Name))
    {
        return *Object;
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

void FConsoleManager::FindCandidates(const StringView& CandidateName, TArray<TPair<IConsoleObject*, String>>& OutCandidates)
{
    for (const auto& Object : ConsoleObjects)
    {
        int32 Length = CandidateName.Length();
        if (Length <= Object.First.Length())
        {
            const CHAR* Command = *Object.First;
            const CHAR* WordIt  = *CandidateName;

            int32 CharDiff = -1;
            while (Length > 0 && (CharDiff = (toupper(*WordIt) - toupper(*Command))) == 0)
            {
                Command++;
                WordIt++;
                Length--;
            }

            if (CharDiff == 0)
            {
                OutCandidates.Emplace(Object.Second, Object.First);
            }
        }
    }
}

void FConsoleManager::ExecuteCommand(IOutputDevice& OutputDevice, const String& Command)
{
    OutputDevice.Log(ELogSeverity::Info, Command);

    History.Emplace(Command);
    if (History.Size() > HistoryLength)
    {
        History.RemoveAt(0);
    }

    int32 Pos = Command.FindChar(' ');
    if (Pos == String::InvalidIndex)
    {
        IConsoleCommand* CommandObject = FindConsoleCommand(*Command);
        if (!CommandObject)
        {
            OutputDevice.Log(ELogSeverity::Error, "'" + Command + "' is not a registered command");
        }
        else
        {
            CommandObject->Execute(StringView());
        }
        
        return;
    }

    const String CommandName(*Command, Pos);
    const int32 ArgsOffset = Pos + 1;
    StringView Args(*Command + ArgsOffset, Command.Length() - ArgsOffset);
    StringView TrimmedArgs = Args;
    TrimmedArgs.TrimInline();

    if (IConsoleCommand* CommandObject = FindConsoleCommand(*CommandName))
    {
        CommandObject->Execute(TrimmedArgs);
        return;
    }

    IConsoleVariable* VariableObject = FindConsoleVariable(*CommandName);
    if (!VariableObject)
    {
        OutputDevice.Log(ELogSeverity::Error, "'" + Command + "' is not a registered command or variable");
        return;
    }

    const String Value(TrimmedArgs);
    const EConsoleVariableFlags SetByConsole = EConsoleVariableFlags::SetByConsole;

    bool bHandled = false;

    int64 Int64Value = 0;
    if (TTypeFromString<int64>::FromString(Value, Int64Value))
    {
        if (VariableObject->IsVariableInt())
        {
            VariableObject->SetAsInt(static_cast<int32>(Int64Value), SetByConsole);
            bHandled = true;
        }
        else if (VariableObject->IsVariableFloat())
        {
            VariableObject->SetAsFloat(static_cast<float>(Int64Value), SetByConsole);
            bHandled = true;
        }
        else if (VariableObject->IsVariableBool())
        {
            VariableObject->SetAsBool(Int64Value != 0, SetByConsole);
            bHandled = true;
        }
        else if (VariableObject->IsVariableString())
        {
            VariableObject->SetString(Value, SetByConsole);
            bHandled = true;
        }
    }

    if (!bHandled && VariableObject->IsVariableFloat())
    {
        float FloatValue = 0.0f;
        if (TTypeFromString<float>::FromString(Value, FloatValue))
        {
            VariableObject->SetAsFloat(FloatValue, SetByConsole);
            bHandled = true;
        }
    }

    if (!bHandled && VariableObject->IsVariableBool())
    {
        bool bBoolValue = false;
        if (TTypeFromString<bool>::FromString(Value, bBoolValue))
        {
            VariableObject->SetAsBool(bBoolValue, SetByConsole);
            bHandled = true;
        }
    }

    if (!bHandled && VariableObject->IsVariableString())
    {
        VariableObject->SetString(Value, SetByConsole);
        bHandled = true;
    }

    if (!bHandled)
    {
        OutputDevice.Log(ELogSeverity::Error, "'" + Value + "' Is an invalid value for '" + CommandName + "'");
    }
}

static bool ApplyCommandLineOverride(const CHAR* InName, IConsoleVariable* Variable)
{
    StringView CommandLineValue;
    if (!CommandLine::FindOption(InName, CommandLineValue))
    {
        return false;
    }

    if (CommandLineValue.IsEmpty())
    {
        // A bare switch means 'on'; SetAsBool maps sensibly onto all four variable types
        Variable->SetAsBool(true, EConsoleVariableFlags::SetByCommandLine);
    }
    else
    {
        Variable->SetString(String(CommandLineValue), EConsoleVariableFlags::SetByCommandLine);
    }

    LOG_INFO("Set ConsoleVariable '%s' from the CommandLine", InName);
    return true;
}

IConsoleObject* FConsoleManager::RegisterObject(const CHAR* InName, IConsoleObject* Object)
{
    const String Name(InName);
    if (IConsoleObject** ExistingObject = ConsoleObjects.Find(Name))
    {
        LOG_WARNING("Trying to register an already existing ConsoleObject '%s'", InName);
        return *ExistingObject;
    }

    IConsoleObject* Result = ConsoleObjects.Add(Name, Object);

    // TODO: Refactor this, right now it only works with a single ConfigFile
    if (IConsoleVariable* Variable = Object->AsVariable())
    {
        if (!ApplyCommandLineOverride(InName, Variable) && GConfig)
        {
            String Value;
            if (GConfig->GetString("", InName, Value))
            {
                Variable->SetString(Value, EConsoleVariableFlags::SetByConfigFile);
            }
        }
    }

    LOG_INFO("Registered ConsoleObject '%s'", *Name);
    return Result;
}

void FConsoleManager::LoadConsoleVariablesFromCommandLine()
{
    for (const auto& Pair : ConsoleObjects)
    {
        if (IConsoleVariable* Variable = Pair.Second ? Pair.Second->AsVariable() : nullptr)
        {
            ApplyCommandLineOverride(*Pair.First, Variable);
        }
    }
}

void FConsoleManager::GetConsoleObjects(TArray<TPair<String, IConsoleObject*>>& OutObjects) const
{
    OutObjects.Clear();
    OutObjects.Reserve(ConsoleObjects.Size());

    for (const auto& Pair : ConsoleObjects)
    {
        OutObjects.Add(TPair<String, IConsoleObject*>(Pair.First, Pair.Second));
    }
}

void FConsoleManager::DumpConsoleVariableValues(IOutputDevice& OutputDevice, const CHAR* Key)
{
    const bool bHasKey = Key && (*Key != '\0');

    TArray<TPair<String, IConsoleObject*>> ConsoleObjectPairs;
    GetConsoleObjects(ConsoleObjectPairs);

    TArray<TPair<String, IConsoleVariable*>> ConsoleVariables;
    ConsoleVariables.Reserve(ConsoleObjectPairs.Size());

    for (const TPair<String, IConsoleObject*>& Pair : ConsoleObjectPairs)
    {
        if (bHasKey)
        {
            const StringView NameView(Pair.First);
            if (NameView.Find(Key, EStringCaseType::NoCase) == StringView::InvalidIndex)
            {
                continue;
            }
        }

        if (IConsoleVariable* Variable = Pair.Second ? Pair.Second->AsVariable() : nullptr)
        {
            ConsoleVariables.Add(TPair<String, IConsoleVariable*>(Pair.First, Variable));
        }
    }

    ConsoleVariables.SortWithPredicate([](const TPair<String, IConsoleVariable*>& A, const TPair<String, IConsoleVariable*>& B)
    {
        return A.First < B.First;
    });

    OutputDevice.Log("CVar Dump");
    OutputDevice.Log(String::CreateFormatted("Count: %d", ConsoleVariables.Size()));
    OutputDevice.Log("----------------------------------------");

    for (const TPair<String, IConsoleVariable*>& Pair : ConsoleVariables)
    {
        IConsoleVariable* Variable = Pair.Second;
        String ValueString;
        const CHAR* TypeString = "unknown";

        if (Variable->IsVariableInt())
        {
            ValueString = String::CreateFormatted("%d", Variable->GetInt());
            TypeString = "int";
        }
        else if (Variable->IsVariableFloat())
        {
            ValueString = String::CreateFormatted("%.6f", Variable->GetFloat());
            TypeString = "float";
        }
        else if (Variable->IsVariableBool())
        {
            ValueString = Variable->GetBool() ? "true" : "false";
            TypeString = "bool";
        }
        else if (Variable->IsVariableString())
        {
            ValueString = Variable->GetString();
            TypeString = "string";
        }

        const EConsoleVariableFlags SetByFlags = Variable->GetFlags() & EConsoleVariableFlags::SetByMask;
        const CHAR* SetByString = SetByFlagToString(SetByFlags);

        OutputDevice.Log(String::CreateFormatted("%s = %s [type:%s setby:%s]", *Pair.First, *ValueString, TypeString, SetByString));
    }
}
