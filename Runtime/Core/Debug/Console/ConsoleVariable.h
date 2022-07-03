#pragma once
#include "IConsoleVariable.h"

#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Containers/String.h"

#include <cstdlib>
#include <sstream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FConsoleVariable

class FConsoleVariable : public IConsoleVariable
{
public:

    FConsoleVariable()
        : IConsoleVariable()
        , ChangedDelegate()
    { }

    FConsoleVariable(const FCVarChangedDelegateType& VariableChangedDelegate)
        : IConsoleVariable()
        , ChangedDelegate()
    {
        ChangedDelegate.Add(VariableChangedDelegate);
    }

    virtual ~FConsoleVariable() = default;

    virtual IConsoleCommand*  AsCommand()  override { return nullptr; }
    virtual IConsoleVariable* AsVariable() override { return this; }

    virtual FCVarChangedDelegate& GetChangedDelegate() override
    {
        return ChangedDelegate;
    }

protected:
    FCVarChangedDelegate ChangedDelegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TConsoleVariable - Templated console-variable for storing a specific type of variable

template<typename T>
class TConsoleVariable : public FConsoleVariable
{
public:
	
    TConsoleVariable()
        : FConsoleVariable()
        , Value()
    { }

    TConsoleVariable(T StartValue)
        : FConsoleVariable()
        , Value(StartValue)
    { }

    TConsoleVariable(T StartValue, const FCVarChangedDelegateType& VariableChangedDelegate)
        : FConsoleVariable(VariableChangedDelegate)
        , Value(StartValue)
    { }

    virtual bool IsInt()    const override final { return false; }
    virtual bool IsFloat()  const override final { return false; }
    virtual bool IsBool()   const override final { return false; }
    virtual bool IsString() const override final { return false; }

    virtual void    SetString(const FString& InValue) override;
    virtual FString GetString() const override;

    virtual void SetInt(int32 InValue) override final
    {
        Value = static_cast<T>(InValue);
        OnChanged();
    }

    virtual void SetFloat(float InValue) override final
    {
        Value = static_cast<T>(InValue);
        OnChanged();
    }

    virtual void SetBool(bool bValue) override final
    {
        Value = static_cast<T>(bValue);
        OnChanged();
    }

    virtual int32 GetInt()   const override final { return static_cast<int32>(Value); }
    virtual float GetFloat() const override final { return static_cast<float>(Value); }
    virtual bool  GetBool()  const override final { return static_cast<bool>(Value); }

private:
    
    FORCEINLINE void OnChanged()
    {
        if (ChangedDelegate.IsBound())
        {
            ChangedDelegate.Broadcast(this);
        }
    }

    T Value;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Int32 - Specialization

template<>
inline void TConsoleVariable<int32>::SetString(const FString& InValue)
{
    Value = atoi(InValue.CStr());
    OnChanged();
}

template<>
inline bool TConsoleVariable<int32>::IsInt() const
{
    return true;
}

template<>
inline FString TConsoleVariable<int32>::GetString() const
{
    FString String;
    String.Format("%d", Value);
    return String;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Float - Specialization

template<>
inline void TConsoleVariable<float>::SetString(const FString& InValue)
{
    Value = (float)atof(InValue.CStr());
    OnChanged();
}

template<>
inline bool TConsoleVariable<float>::IsFloat() const
{
    return true;
}

template<>
inline FString TConsoleVariable<float>::GetString() const
{
    FString String;
    String.Format("%.4f", Value);
    return String;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Bool - Specialization

template<>
inline void TConsoleVariable<bool>::SetString(const FString& InValue)
{
    FString Lower = InValue.ToLower();

    int32 Number = 0;
    std::istringstream Stream(Lower.CStr());
    Stream >> Number;

    if (Stream.fail())
    {
        Stream.clear();
        Stream >> std::boolalpha >> Value;
    }
    else
    {
        Value = static_cast<bool>(Number);
    }

    if (!Stream.fail())
    {
        OnChanged();
    }
}

template<>
inline FString TConsoleVariable<bool>::GetString() const
{
    return Value ? "true" : "false";
}

template<>
inline bool TConsoleVariable<bool>::IsBool() const
{
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// String - Specialization

template<>
inline void TConsoleVariable<FString>::SetInt(int32 InValue)
{
    Value.Format("%d", InValue);
}

template<>
inline void TConsoleVariable<FString>::SetFloat(float InValue)
{
    Value.Format("%.4f", InValue);
}

template<>
inline void TConsoleVariable<FString>::SetBool(bool InValue)
{
    // TODO: Change to the ToString

    std::stringstream Stream;
    Stream << std::boolalpha << InValue;
    Value = Stream.str().c_str();
}

template<>
inline void TConsoleVariable<FString>::SetString(const FString& InValue)
{
    Value = InValue;
    OnChanged();
}

template<>
inline bool TConsoleVariable<FString>::IsString() const
{
    return true;
}

template<>
inline int32 TConsoleVariable<FString>::GetInt() const
{
    return 0;
}

template<>
inline float TConsoleVariable<FString>::GetFloat() const
{
    return 0.0f;
}

template<>
inline bool TConsoleVariable<FString>::GetBool() const
{
    return false;
}

template<>
inline FString TConsoleVariable<FString>::GetString() const
{
    return Value;
}
