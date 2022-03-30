#pragma once

#if PLATFORM_MACOS
#include "MacCursor.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Containers/Array.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Interface/PlatformApplication.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

@class CCocoaAppDelegate;
@class CCocoaWindow;

class CMacWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Holds information about an event or notification that occurs when calling PumpMessages and CMacApplication::Tick

// TODO: Finish
struct SMacApplicationEvent
{
    FORCEINLINE SMacApplicationEvent()
        : NotificationName(nullptr)
        , Event(nullptr)
        , Window(nullptr)
        , Size()
        , Position()
        , Character(uint32(-1))
    { }

    FORCEINLINE SMacApplicationEvent(const SMacApplicationEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nullptr)
        , Event(Other.Event ? [Other.Event retain] : nullptr)
        , Window(Other.Window ? [Other.Window retain] : nullptr)
        , Size(Other.Size)
        , Position(Other.Position)
        , Character(Other.Character)
    { }

    FORCEINLINE ~SMacApplicationEvent()
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (NotificationName)
        {
            [NotificationName release];
        }

        if (Event)
        {
            [Event release];
        }

        if (Window)
        {
            [Window release];
        }
    }

    // Name of notification, nullptr if not a notification
    NSNotificationName NotificationName;
    // Event object, nullptr if not an event
    NSEvent* Event;
    
    // Window for the event, nullptr if no associated window exists
    CCocoaWindow* Window;
    // Size of the window
    CGSize Size;
    // Position of the window
    CGPoint Position;
    
    // On Character typed event
    uint32 Character;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacApplication - Mac specific implementation of the application interface

class CMacApplication final : public CPlatformApplication
{
public:
    
    /*//////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformApplication Interface
    
    static TSharedPtr<CMacApplication> CreateApplication();

    virtual TSharedRef<CPlatformWindow> MakeWindow() override final;

    virtual bool Initialize() override final;

    virtual void Tick(float Delta) override final;

    virtual void SetActiveWindow(const TSharedRef<CPlatformWindow>& Window) override final;
    
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const override final;
    virtual TSharedRef<CPlatformWindow> GetWindowUnderCursor() const override final;
    
public:
    
    /**
     * @brief: Retrieve window object from a NSWindow object
     *
     * @param Window: Native window object to search for
     * @return: Returns the MacWindow accociated with the NSWindow object
     */
    TSharedRef<CMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;

    /**
     * @brief: Store event for handling later in the main loop
     *
     * @param EventOrNotificationObject: Native window object to search for
     */
    void DeferEvent(NSObject* EventOrNotificationObject);
    
    /**
     * @brief: Returns the native appdelegate
     *
     * @return: Returns the application delegate
     */
    FORCEINLINE CCocoaAppDelegate* GetAppDelegate() const
    {
        return AppDelegate;
    }

private:
    
    friend struct TDefaultDelete<CMacApplication>;

    CMacApplication();
    ~CMacApplication();
    
    bool InitializeAppMenu();

    void HandleEvent(const SMacApplicationEvent& Notification);

    CCocoaAppDelegate* AppDelegate = nullptr;

    TArray<TSharedRef<CMacWindow>> Windows;
    mutable CCriticalSection       WindowsMutex;

    TArray<SMacApplicationEvent> DeferredEvents;
    CCriticalSection             DeferredEventsMutex;

    bool bIsTerminating = false;
};

#endif
