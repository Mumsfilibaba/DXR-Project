#pragma once
#include "PlatformApplicationMessageHandler.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/ICursor.h"

#ifdef GetMonitorInfo
#undef GetMonitorInfo
#endif

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

class CPlatformApplication
{
public:

    /* Creates the application */
    static TSharedPtr<CPlatformApplication> Make() { return TSharedPtr<CPlatformApplication>(); }

    /* Public destructor for TSharedPtr */
    virtual ~CPlatformApplication() = default;

    /* Create a window */
    virtual TSharedRef<CPlatformWindow> MakeWindow() { return TSharedRef<CPlatformWindow>(); }

    /* Initialized the application */
    virtual bool Initialize() { return true; }

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) {}

    /* Returns true if the platform supports Raw mouse movement */
    virtual bool SupportsRawMouse() const { return false; }

    /* Enables Raw mouse movement for a certain window */
	virtual bool EnableRawMouse( const TSharedRef<CPlatformWindow>& Window ) { return true; }

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CPlatformWindow>& Window ) {}

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const { return TSharedRef<CPlatformWindow>(); }

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CPlatformWindow>& ) {}

    /* Retrieves the window that currently has the keyboard focus, since macOS does not support keyboard focus, we return null as standard */
    virtual TSharedRef<CPlatformWindow> GetCapture() const { return TSharedRef<CPlatformWindow>(); }

    /* Retrieve the window that is currently under the cursor, if no window is under the cursor, the value is nullptr */
    virtual TSharedRef<CPlatformWindow> GetWindowUnderCursor() const { return TSharedRef<CPlatformWindow>(); } 

    /* Sets the message handler */
    virtual void SetMessageListener( const TSharedPtr<CPlatformApplicationMessageHandler>& InMessageHandler ) { MessageListener = InMessageHandler; }

    /* Retrieve the cursor interface */
    FORCEINLINE TSharedPtr<ICursor> GetCursor() const
    { 
        return Cursor;
    }

    /* Retrieves the message handler */
    FORCEINLINE TSharedPtr<CPlatformApplicationMessageHandler> GetMessageListener() const
    { 
        return MessageListener;
    }

protected:

    /* Protected constructor, use Make */
    CPlatformApplication( const TSharedPtr<ICursor>& InCursor )
        : Cursor( InCursor )
        , MessageListener( nullptr )
    {
    }

    /* The cursor interface for the application */
    TSharedPtr<ICursor> Cursor;

    /* Handler for platform messages/events */
    TSharedPtr<CPlatformApplicationMessageHandler> MessageListener;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif