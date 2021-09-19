#pragma once
#include "Core/Application/ICursor.h"
#include "Core/Application/IKeyboard.h"
#include "Core/Containers/SharedPtr.h"

#include "GenericApplicationMessageListener.h"

// TODO: Remove
#include <string>

/* Generic application interface */
class CGenericApplication
{
public: 

    /* Creates the mac application */
    static FORCEINLINE CGenericApplication* Make()
    {
        return nullptr;
    }

    /* Create a window */
    virtual CGenericWindow* MakeWindow() = 0;

    /* Initialized the application */
    virtual bool Init() = 0;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) = 0;

    /* Releases the application */
    virtual void Release() = 0;

    /* Retrive the cursor interface */
    virtual ICursor* GetCursor() = 0;

    /* Retrive the keyboard interface */
    virtual IKeyboard* GetKeyboard() = 0;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( CGenericWindow* )
	{
	}

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( CGenericWindow* Window ) = 0;

    /* Retrives the window that currently has the keyboard focus, since macOS does not support keyboard focus, we return null as standard */
    virtual CGenericWindow* GetCapture() const 
    {
        return nullptr;
    }

    /* Retrives the window that is currently active */
    virtual CGenericWindow* GetActiveWindow() const = 0;

    /* Sets the message listener */
    FORCEINLINE void SetMessageListener( const TSharedPtr<CGenericApplicationMessageListener>& InMessageListener )
    {
        MessageListener = InMessageListener;
    }

    /* Retrives the message listener */
    FORCEINLINE TSharedPtr<CGenericApplicationMessageListener> GetMessageListener() const
    {
        return MessageListener;
    }

protected:
    
    CGenericApplication()  = default;
    ~CGenericApplication() = default;

    TSharedPtr<CGenericApplicationMessageListener> MessageListener;
};
