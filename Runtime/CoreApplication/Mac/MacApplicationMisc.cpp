#if PLATFORM_MACOS && defined(__OBJC__)
#include "MacApplicationMisc.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Input/InputCodes.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacApplicationMisc

void CMacApplicationMisc::MessageBox(const String& Title, const String& Message)
{
    const char* RawTitle   = Title.CStr();
    const char* RawMessage = Message.CStr();
    
    CFStringRef CaptionRef = CFStringCreateWithCString(0, RawTitle  , static_cast<CFStringEncoding>(Title.Length()));
    CFStringRef TextRef    = CFStringCreateWithCString(0, RawMessage, static_cast<CFStringEncoding>(Message.Length()));
        
    CFOptionFlags Result = 0;
    CFOptionFlags Flags  = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert(0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result);
    
    CFRelease(CaptionRef);
    CFRelease(TextRef);
}

void CMacApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    SCOPED_AUTORELEASE_POOL();
    
    Assert(NSApp != nullptr);
    
    NSEvent* Event = nullptr;
    do
    {
        Event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (!Event)
        {
            break;
        }
        
        [NSApp sendEvent:Event];
    } while (bUntilEmpty);
    
    // HACK: Look into a better solution in the future
    PlatformThreadMisc::RunMainLoop();
}

SModifierKeyState CMacApplicationMisc::GetModifierKeyState()
{
    NSUInteger CurrentModifiers = ([NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask);

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
        
    return SModifierKeyState(Mask);
}

#endif
