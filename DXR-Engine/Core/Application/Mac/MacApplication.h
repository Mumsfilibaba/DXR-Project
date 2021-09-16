#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericApplication.h"
#include "Core/Containers/Array.h"

#if defined(__OBJC__)
@class CocoaAppDelegate;
#else
class CocoaAppDelegate;
#endif

/* Mac specific implementation of the application */
class CMacApplication final : public CGenericApplication
{
public:

    CMacApplication();
    ~CMacApplication();

    /* Create a window */
    virtual GenericWindow* MakeWindow( const std::string& Title, uint32 Width, uint32 Height, struct SWindowStyle& Style ) override final;

    /* Initialized the application */
    virtual bool Init() override final;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) override final;

    /* Releases the application */
    virtual void Release() override final;

    /* Retrive the cursor interface */
    virtual ICursor* GetCursor() override final;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( GenericWindow* Window ) override final;

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( GenericWindow* Window ) override final;

    /* Retrives the window that is currently active */
    virtual GenericWindow* GetActiveWindow() override final;

private:

    /* Delegate to talk with macOS */
	CocoaAppDelegate* AppDelegate = nullptr;

    /* All the windows of the application */
    TArray<TSharedRef<GenericWindow>> Windows;
};

#endif