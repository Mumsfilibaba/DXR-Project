#pragma once
#include "Core.h"

#include "Core/Delegates/MulticastDelegate.h"

class IConsoleObject
{
public:
    virtual class IConsoleVariable* AsVariable() = 0;
    virtual class IConsoleCommand* AsCommand() = 0;
};

DECLARE_MULTICAST_DELEGATE( CChangedDelegate, IConsoleVariable* );

class IConsoleVariable : public IConsoleObject
{
public:
    virtual void SetInt( int32 InValue ) = 0;
    virtual void SetFloat( float InValue ) = 0;
    virtual void SetBool( bool InValue ) = 0;
    virtual void SetString( const CString& InValue ) = 0;

    virtual int32 GetInt() const = 0;
    virtual float GetFloat() const = 0;
    virtual bool GetBool() const = 0;
    virtual CString GetString() const = 0;

    virtual bool IsInt() const = 0;
    virtual bool IsFloat() const = 0;
    virtual bool IsBool() const = 0;
    virtual bool IsString() const = 0;

    virtual CChangedDelegate& GetChangedDelegate() = 0;
};

DECLARE_MULTICAST_DELEGATE( CExecutedDelegate );

class IConsoleCommand : public IConsoleObject
{
public:

    // TODO: Add parameter-list
    virtual void Execute() = 0;

    /* Retrieve the delegate that gets broadcasted when the console executes the command */
    virtual CExecutedDelegate& GetExecutedDelgate() = 0;
};