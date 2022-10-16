#pragma once
#include "IConsoleObject.h"

#include "Core/Containers/String.h"
#include "Core/Delegates/MulticastDelegate.h"

DECLARE_MULTICAST_DELEGATE(FCVarChangedDelegate, struct IConsoleVariable*);

struct IConsoleVariable 
    : public IConsoleObject
{
    /**
     * @brief         - Set the variable with an int
     * @param InValue - Value to store
     */
    virtual void SetInt(int32 InValue) = 0;

    /**
     * @brief         - Set the variable with an float
     * @param InValue - Value to store
     */
    virtual void SetFloat(float InValue) = 0;

    /**
     * @brief        - Set the variable with an bool
     * @param bValue - Value to store
     */
    virtual void SetBool(bool bValue) = 0;

    /**
     * @brief         - Set the variable with an string
     * @param InValue - Value to store
     */
    virtual void SetString(const FString& InValue) = 0;
    
    /**
     * @brief  - Retrieve the variable as an int
     * @return - Returns an int with the value of the variable
     */
    virtual int32 GetInt() const = 0;

    /**
     * @brief  - Retrieve the variable as an float
     * @return - Returns an float with the value of the variable
     */
    virtual float GetFloat() const = 0;

    /**
     * @brief  - Retrieve the variable as an bool
     * @return - Returns an bool with the value of the variable
     */
    virtual bool GetBool() const = 0;

    /**
     * @brief  - Retrieve the variable as an string
     * @return - Returns an string with the value of the variable
     */
    virtual FString GetString() const = 0;
    
    /**
     * @brief  - Check weather the variable is an int
     * @return - Returns true if the variable is an int
     */
    virtual bool IsInt() const = 0;

    /**
     * @brief  - Check weather the variable is a float
     * @return - Returns true if the variable is a float
     */
    virtual bool IsFloat() const = 0;

    /**
     * @brief  - Check weather the variable is a bool
     * @return - Returns true if the variable is a bool
     */
    virtual bool IsBool() const = 0;

    /**
     * @brief  - Check weather the variable is a string
     * @return - Returns true if the variable is a string
     */
    virtual bool IsString() const = 0;

    /**
     * @brief  - Retrieve the delegate that gets called when the variable changes
     * @return - Returns the on changed delegate
     */
    virtual FCVarChangedDelegate& GetChangedDelegate() = 0;
};