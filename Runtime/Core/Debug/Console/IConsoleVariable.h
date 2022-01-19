#pragma once
#include "IConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Multi-cast delegate to be called when the variable changes

DECLARE_MULTICAST_DELEGATE(CChangedDelegate, IConsoleVariable*);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Base console-variable 

class IConsoleVariable : public IConsoleObject
{
public:

    /* Check if variable is an int */
    virtual bool IsInt() const = 0;
    /* Check if variable is an float */
    virtual bool IsFloat() const = 0;
    /* Check if variable is an bool */
    virtual bool IsBool() const = 0;
    /* Check if variable is an string */
    virtual bool IsString() const = 0;

    /* Set as int */
    virtual void SetInt(int32 InValue) = 0;
    /* Set as float */
    virtual void SetFloat(float InValue) = 0;
    /* Set as bool  */
    virtual void SetBool(bool bValue) = 0;
    /* Set as string */
    virtual void SetString(const CString& InValue) = 0;

    /* Retrieve as int */
    virtual int32 GetInt() const = 0;
    /* Retrieve as float */
    virtual float GetFloat() const = 0;
    /* Retrieve as bool */
    virtual bool GetBool() const = 0;
    /* Retrieve as string */
    virtual CString GetString() const = 0;

    /* Delegate that gets broadcasted when the variable changes */
    virtual CChangedDelegate& GetChangedDelegate() = 0;
};