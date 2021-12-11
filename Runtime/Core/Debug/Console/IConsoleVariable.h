#pragma once
#include "IConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Containers/String.h"

DECLARE_MULTICAST_DELEGATE( CChangedDelegate, IConsoleVariable* );

class IConsoleVariable : public IConsoleObject
{
public:
    virtual void SetInt( int32 InValue ) = 0;
    virtual void SetFloat( float InValue ) = 0;
    virtual void SetBool( bool bValue ) = 0;
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