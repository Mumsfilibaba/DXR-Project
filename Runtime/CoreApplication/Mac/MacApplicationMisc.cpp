#if PLATFORM_MACOS && defined(__OBJC__)
#include "MacApplicationMisc.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Input/InputCodes.h"

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

void CMacApplicationMisc::MessageBox( const CString& Title, const CString& Message )
{
    CFStringRef CaptionRef = CFStringCreateWithCString( 0, Title.CStr(),   static_cast<CFStringEncoding>(Title.Length()) );
    CFStringRef TextRef    = CFStringCreateWithCString( 0, Message.CStr(), static_cast<CFStringEncoding>(Message.Length()) );
        
    CFOptionFlags Result = 0;
    CFOptionFlags Flags  = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert( 0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result );
    
    CFRelease( CaptionRef );
    CFRelease( TextRef );
}

void CMacApplicationMisc::RequestExit( int32 ExitCode )
{
    [NSApp terminate:nil];
}

void CMacApplicationMisc::PumpMessages( bool UntilEmpty )
{
    SCOPED_AUTORELEASE_POOL();
    
    Assert( NSApp != nullptr );
    
    NSEvent* Event = nullptr;
    do
    {
        Event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (!Event)
        {
            break;
        }
        
        [NSApp sendEvent:Event];
    } while ( UntilEmpty );
}

SModifierKeyState CMacApplicationMisc::GetModifierKeyState()
{
    NSUInteger CurrentModifiers = ([NSEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask);

    uint32 Mask = 0;
    if (CurrentModifiers & NSEventModifierFlagControl)
    {
        Mask |= EModifierFlag::ModifierFlag_Ctrl;
    }
    if (CurrentModifiers & NSEventModifierFlagShift)
    {
        Mask |= EModifierFlag::ModifierFlag_Shift;
    }
    if (CurrentModifiers & NSEventModifierFlagOption)
    {
        Mask |= EModifierFlag::ModifierFlag_Alt;
    }
    if (CurrentModifiers & NSEventModifierFlagCommand)
    {
        Mask |= EModifierFlag::ModifierFlag_Super;
    }
    if (CurrentModifiers & NSEventModifierFlagCapsLock)
    {
        Mask |= EModifierFlag::ModifierFlag_CapsLock;
    }
        
    return SModifierKeyState( Mask );
}

#endif