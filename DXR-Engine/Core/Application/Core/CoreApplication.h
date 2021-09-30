#pragma once
#include "Core/Application/ICursorDevice.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplicationMessageHandler.h"

// TODO: Remove
#include <string>

/* Generic application interface */
class CCoreApplication
{
public:

    virtual ~CCoreApplication() = default;

    /* Creates the mac application */
    static FORCEINLINE TSharedPtr<CCoreApplication> Make()
    {
        return nullptr;
    }

    /* Create a window */
    virtual TSharedRef<CCoreWindow> MakeWindow() = 0;

    /* Initialized the application */
    virtual bool Init() = 0;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) = 0;

    /* Retrieve the cursor interface */
    virtual ICursorDevice* GetCursor() = 0;

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CCoreWindow>& Window ) = 0;

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CCoreWindow> GetActiveWindow() const = 0;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CCoreWindow>& )
    {
    }

    /* Retrieves the window that currently has the keyboard focus, since macOS does not support keyboard focus, we return null as standard */
    virtual TSharedRef<CCoreWindow> GetCapture() const
    {
        return nullptr;
    }

    /* Sets the message handler */
    FORCEINLINE void SetMessageListener( const TSharedPtr<CCoreApplicationMessageHandler>& InMessageHandler )
    {
        MessageListener = InMessageHandler;
    }

    /* Retrieves the message handler */
    FORCEINLINE TSharedPtr<CCoreApplicationMessageHandler> GetMessageListener() const
    {
        return MessageListener;
    }

protected:

    /* Protected constructor, use Make */
    CCoreApplication() = default;

    /* Handler for platform messages/events */
    TSharedPtr<CCoreApplicationMessageHandler> MessageListener;
};
