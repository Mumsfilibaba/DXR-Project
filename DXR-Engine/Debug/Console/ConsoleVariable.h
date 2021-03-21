#pragma once
#include "ConsoleObject.h"

#include <string>

enum class EConsoleVariableType : uint8
{
    Unknown = 0,
    Bool    = 1,
    Int     = 2,
    Float   = 3,
    String  = 4,
};

using String = std::string;

class ConsoleVariable : public ConsoleObject
{
public:
    virtual ConsoleVariable* AsVariable() override { return this; }

    virtual EConsoleVariableType GetType() const = 0;

    virtual void SetInt(int32 InVariable)
    {
        Assert(false);
    }

    virtual void SetBool(bool InVariable)
    {
        Assert(false);
    }

    virtual void SetFloat(float InVariable)
    {

    }

    virtual void SetString(const String& InVariable) = 0;

    virtual int32 GetInt() const = 0;
    virtual bool GetBool() const = 0;
    virtual float GetFloat() const = 0;
    virtual const String& GetString() const = 0;

};

template<typename T>
class TConsoleVariable;

template<>
class TConsoleVariable<int32> : public ConsoleVariable
{
public:
    TConsoleVariable(int32 InVariable)
        : ConsoleVariable()
        , Variable(InVariable)
    {
    }

    virtual void SetInt(int32 InVariable) override
    {
        Variable = InVariable;
    }

    virtual int32 GetInt() const override
    {
        return Variable;
    }

private:
    int32 Variable;
};

template<>
class TConsoleVariable<bool> : public ConsoleVariable
{
public:
    TConsoleVariable(bool InVariable)
        : ConsoleVariable()
        , Variable(InVariable)
    {
    }

    virtual void SetInt(int32 InVariable) override
    {
        Variable = (bool)InVariable;
    }

    virtual void SetBool(bool InVariable) override
    {
        Variable = InVariable;
    }

    virtual bool GetBool() const override
    {
        return Variable;
    }

private:
    bool Variable;
};

template<>
class TConsoleVariable<float> : public ConsoleVariable
{
public:
    TConsoleVariable(float InVariable)
        : ConsoleVariable()
        , Variable(InVariable)
    {
    }

    virtual void SetInt(int32 InVariable) override
    {
        Variable = (float)InVariable;
    }

    virtual void SetFloat(float InVariable) override
    {
        Variable = InVariable;
    }

    virtual float GetFloat() const override
    {
        return Variable;
    }

private:
    float Variable;
};

template<>
class TConsoleVariable<String> : public ConsoleVariable
{
public:
    TConsoleVariable(const String& InVariable)
        : ConsoleVariable()
        , Variable(InVariable)
    {
    }

    virtual void SetString(const String& InVariable) override
    {
        Variable = InVariable;
    }

    virtual const String& GetString() const override
    {
        return Variable;
    }

private:
    String Variable;
};