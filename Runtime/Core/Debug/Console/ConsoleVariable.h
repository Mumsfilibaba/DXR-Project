#pragma once
#include "IConsoleVariable.h"

#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Containers/String.h"

#include <cstdlib>
#include <sstream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleVariable - BaseClass for a console-variable that contains a delegate for when the variable changes

class CConsoleVariable : public IConsoleVariable
{
public:

    CConsoleVariable()
        : IConsoleVariable()
        , ChangedDelegate()
    {
    }

    CConsoleVariable(const CConsoleVariableChangedDelegateType& VariableChangedDelegate)
        : IConsoleVariable()
        , ChangedDelegate()
    {
        ChangedDelegate.Add(VariableChangedDelegate);
    }

    virtual ~CConsoleVariable() = default;

    virtual IConsoleCommand* AsCommand() override { return nullptr; }
    virtual IConsoleVariable* AsVariable() override { return this; }

    virtual CConsoleVariableChangedDelegate& GetChangedDelegate() override
    {
        return ChangedDelegate;
    }

protected:
    CConsoleVariableChangedDelegate ChangedDelegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Templated console-variable for storing a specific type of variable

template<typename T>
class TConsoleVariable : public CConsoleVariable
{
public:
    TConsoleVariable()
        : CConsoleVariable()
        , Value()
    {
    }

    TConsoleVariable(T StartValue)
        : CConsoleVariable()
        , Value(StartValue)
    {
    }

    TConsoleVariable(T StartValue, const CConsoleVariableChangedDelegateType& VariableChangedDelegate)
        : CConsoleVariable(VariableChangedDelegate)
        , Value(StartValue)
    {
    }

    virtual bool IsInt() const override final { return false; }
    virtual bool IsFloat() const override final { return false; }
    virtual bool IsBool() const override final { return false; }
    virtual bool IsString() const override final { return false; }

    virtual void SetString(const CString& InValue) override;
    virtual CString GetString() const override;

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

    virtual int32 GetInt() const override final
    {
        return static_cast<int32>(Value);
    }

    virtual float GetFloat() const override final
    {
        return static_cast<float>(Value);
    }

    virtual bool GetBool() const override final
    {
        return static_cast<bool>(Value);
    }

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
// int32 - Specialization

template<>
inline void TConsoleVariable<int32>::SetString(const CString& InValue)
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
inline CString TConsoleVariable<int32>::GetString() const
{
    CString String;
    String.Format("%d", Value);
    return String;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// float - Specialization

template<>
inline void TConsoleVariable<float>::SetString(const CString& InValue)
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
inline CString TConsoleVariable<float>::GetString() const
{
    CString String;
    String.Format("%.4f", Value);
    return String;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// bool - Specialization

template<>
inline void TConsoleVariable<bool>::SetString(const CString& InValue)
{
    CString Lower = InValue.ToLower();

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
inline CString TConsoleVariable<bool>::GetString() const
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
inline void TConsoleVariable<CString>::SetInt(int32 InValue)
{
    Value.Format("%d", InValue);
}

template<>
inline void TConsoleVariable<CString>::SetFloat(float InValue)
{
    Value.Format("%.4f", InValue);
}

template<>
inline void TConsoleVariable<CString>::SetBool(bool InValue)
{
    // TODO: Change to the ToString

    std::stringstream Stream;
    Stream << std::boolalpha << InValue;
    Value = Stream.str().c_str();
}

template<>
inline void TConsoleVariable<CString>::SetString(const CString& InValue)
{
    Value = InValue;
    OnChanged();
}

template<>
inline bool TConsoleVariable<CString>::IsString() const
{
    return true;
}

template<>
inline int32 TConsoleVariable<CString>::GetInt() const
{
    return 0;
}

template<>
inline float TConsoleVariable<CString>::GetFloat() const
{
    return 0.0f;
}

template<>
inline bool TConsoleVariable<CString>::GetBool() const
{
    return false;
}

template<>
inline CString TConsoleVariable<CString>::GetString() const
{
    return Value;
}
