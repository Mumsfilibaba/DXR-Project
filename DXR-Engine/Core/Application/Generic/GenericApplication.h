#pragma once
#include "Core/Application/ICursorDevice.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#include "GenericApplicationMessageListener.h"

// TODO: Remove
#include <string>

/* Generic application interface */
class CGenericApplication
{
public:

    virtual ~CGenericApplication() = default;

    /* Creates the mac application */
    static FORCEINLINE TSharedPtr<CGenericApplication> Make()
    {
        return nullptr;
    }

    /* Create a window */
    virtual TSharedRef<CGenericWindow> MakeWindow() = 0;

    /* Initialized the application */
    virtual bool Init() = 0;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) = 0;

    /* Retrieve the cursor interface */
    virtual ICursorDevice* GetCursor() = 0;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CGenericWindow>& )
    {
    }

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CGenericWindow>& Window ) = 0;

    /* Retrieves the window that currently has the keyboard focus, since macOS does not support keyboard focus, we return null as standard */
    virtual TSharedRef<CGenericWindow> GetCapture() const
    {
        return nullptr;
    }

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CGenericWindow> GetActiveWindow() const = 0;

    /* Sets the message listener */
    FORCEINLINE void SetMessageListener( const TSharedPtr<CGenericApplicationMessageListener>& InMessageListener )
    {
        MessageListener = InMessageListener;
    }

    /* Retrieves the message listener */
    FORCEINLINE TSharedPtr<CGenericApplicationMessageListener> GetMessageListener() const
    {
        return MessageListener;
    }

protected:

    /* Protected constructor, use Make */
    CGenericApplication() = default;

    /* Listener for platform messages/events */
    TSharedPtr<CGenericApplicationMessageListener> MessageListener;
};
