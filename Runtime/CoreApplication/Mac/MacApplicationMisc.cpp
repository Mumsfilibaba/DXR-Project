#include "MacApplicationMisc.h"
#include "MacApplication.h"
#include "MacConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Input/InputCodes.h"

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

CGenericApplication* CMacApplicationMisc::CreateApplication()
{
    return CMacApplication::CreateMacApplication();
}

CGenericConsoleWindow* CMacApplicationMisc::CreateConsoleWindow()
{
    return CMacConsoleWindow::CreateMacConsole();
}

void CMacApplicationMisc::MessageBox(const String& Title, const String& Message)
{
    CFStringRef CaptionRef = CFStringCreateWithCString(0, Title.CStr(),   static_cast<CFStringEncoding>(Title.Length()));
    CFStringRef TextRef    = CFStringCreateWithCString(0, Message.CStr(), static_cast<CFStringEncoding>(Message.Length()));
        
    CFOptionFlags Result = 0;
    CFOptionFlags Flags  = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert(0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result);
    
    CFRelease(CaptionRef);
    CFRelease(TextRef);
}

void CMacApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    SCOPED_AUTORELEASE_POOL();
    
    Check(NSApp != nullptr);
	
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
