#pragma once
#include "ConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

#include <cstdlib>
#include <sstream>

class ConsoleVariable : public ConsoleObject
{
public:
    virtual ConsoleVariable* AsVariable() override { return this; }

    virtual void SetInt(int32 InValue) = 0;
    virtual void SetFloat(float InValue) = 0;
    virtual void SetBool(bool InValue) = 0;
    virtual void SetString(const String& InValue) = 0;

    virtual int32 GetInt() const = 0;
    virtual float GetFloat() const = 0;
    virtual bool GetBool() const = 0;
    virtual String GetString() const = 0;

    virtual bool IsInt() const = 0;
    virtual bool IsFloat() const = 0;
    virtual bool IsBool() const = 0;
    virtual bool IsString() const = 0;
    
    TMulticastDelegate<ConsoleVariable*> OnChangedDelegate;
};

template<typename T>
class TConsoleVariable : public ConsoleVariable
{
public:
    TConsoleVariable()
        : ConsoleVariable()
        , Value()
    {
    }

    TConsoleVariable(T StartValue)
        : ConsoleVariable()
        , Value(StartValue)
    {
    }

    virtual void SetInt(int32 InValue) override 
    { 
        Value = (T)InValue;
        OnChanged();
    }

    virtual void SetFloat(float InValue) override
    {
        Value = (T)InValue;
        OnChanged();
    }

    virtual void SetBool(bool InValue) override
    {
        Value = (T)InValue;
        OnChanged();
    }

    virtual void SetString(const String& InValue) override;

    virtual int32 GetInt() const override
    {
        return (T)Value;
    }

    virtual float GetFloat() const override
    {
        return (T)Value;
    }

    virtual bool GetBool() const override
    {
        return (T)Value;
    }

    virtual String GetString() const override
    {
        return std::to_string(Value);
    }

    virtual bool IsInt() const override { return false; }
    virtual bool IsFloat() const override { return false; }
    virtual bool IsBool() const override { return false; }
    virtual bool IsString() const override { return false; }

private:
    void OnChanged()
    {
        if (OnChangedDelegate.IsBound())
        {
            OnChangedDelegate.Broadcast(this);
        }
    }

    T Value;
};

// int32
template<> void TConsoleVariable<int32>::SetString(const String& InValue)
{
    Value = atoi(InValue.c_str());
    OnChanged();
}

template<> bool TConsoleVariable<int32>::IsInt() const 
{ 
    return true; 
}

// float
template<> void TConsoleVariable<float>::SetString(const String& InValue)
{
    Value = (float)atof(InValue.c_str());
    OnChanged();
}

template<> bool TConsoleVariable<float>::IsFloat() const
{
    return true;
}

// bool
template<> void TConsoleVariable<bool>::SetString(const String& InValue)
{
    String Lower = InValue;
    for (char& c : Lower)
    {
        c = (char)tolower(c);
    }

    std::istringstream Stream(Lower);
    Stream >> std::boolalpha >> Value;

    if (Stream.fail())
    {
        Stream.clear();
        Stream >> Value;
    }

    if (!Stream.fail())
    {
        OnChanged();
    }
}

template<> String TConsoleVariable<bool>::GetString() const
{
    return Value ? "true" : "false";
}

template<> bool TConsoleVariable<bool>::IsBool() const
{
    return true;
}

// String
template<> void TConsoleVariable<String>::SetInt(int32 InValue)
{
    Value = std::to_string(InValue);
}

template<> void TConsoleVariable<String>::SetFloat(float InValue)
{
    Value = std::to_string(InValue);
}

template<> void TConsoleVariable<String>::SetBool(bool InValue)
{
    std::stringstream Stream;
    Stream << std::boolalpha << InValue;
    Value = Stream.str();
}

template<> void TConsoleVariable<String>::SetString(const String& InValue)
{
    Value = InValue;
}

template<> bool TConsoleVariable<String>::IsString() const
{
    return true;
}
