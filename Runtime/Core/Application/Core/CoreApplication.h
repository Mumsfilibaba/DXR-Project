#pragma once
#include "Core/Application/ICursor.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplicationMessageHandler.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Generic application interface */
class CCoreApplication
{
public:

    virtual ~CCoreApplication() = default;
    
    /* Creates the application */
    static TSharedPtr<CCoreApplication> Make() { return TSharedPtr<CCoreApplication>(); }

    /* Create a window */
    virtual TSharedRef<CCoreWindow> MakeWindow() { return TSharedRef<CCoreWindow>(); }

    /* Initialized the application */
    virtual bool Init() { return true; }

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) {}

    /* Retrieve the cursor interface */
    virtual ICursor* GetCursor() { return nullptr; }

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CCoreWindow>& Window ) {}

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CCoreWindow> GetActiveWindow() const { return TSharedRef<CCoreWindow>(); }

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CCoreWindow>& ) {}

    /* Retrieves the window that currently has the keyboard focus, since macOS does not support keyboard focus, we return null as standard */
    virtual TSharedRef<CCoreWindow> GetCapture() const { return TSharedRef<CCoreWindow>(); }

    /* Sets the message handler */
    virtual void SetMessageListener( const TSharedPtr<CCoreApplicationMessageHandler>& InMessageHandler )
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
    CCoreApplication()
        : MessageListener()
    {
    }

    /* Handler for platform messages/events */
    TSharedPtr<CCoreApplicationMessageHandler> MessageListener;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif