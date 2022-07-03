#include "MacApplicationMisc.h"
#include "MacApplication.h"
#include "MacConsoleWindow.h"

#include "Core/Mac/Mac.h"
#include "Core/Input/InputCodes.h"

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

FGenericApplication* FMacApplicationMisc::CreateApplication()
{
    return FMacApplication::CreateMacApplication();
}

FGenericConsoleWindow* FMacApplicationMisc::CreateConsoleWindow()
{
    return FMacConsoleWindow::CreateMacConsole();
}

void FMacApplicationMisc::MessageBox(const String& Title, const String& Message)
{
    SCOPED_AUTORELEASE_POOL();
    
    CFStringRef CaptionRef = CFStringCreateWithCString(0, Title.CStr(),   static_cast<CFStringEncoding>(Title.Length()));
    CFStringRef TextRef    = CFStringCreateWithCString(0, Message.CStr(), static_cast<CFStringEncoding>(Message.Length()));
        
    CFOptionFlags Result = 0;
    CFOptionFlags Flags  = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert(0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result);
    
    CFRelease(CaptionRef);
    CFRelease(TextRef);
}

void FMacApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    SCOPED_AUTORELEASE_POOL();
    
    Check(NSApp != nil);
	
    NSEvent* Event = nil;
    do
    {
        Event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (!Event)
        {
            break;
        }
        
        // Prevent to send event from invalid windows
        if ([Event windowNumber] == 0 || [Event window] != nil)
        {
            [NSApp sendEvent:Event];
        }
        
    } while (bUntilEmpty);
}

FModifierKeyState FMacApplicationMisc::GetModifierKeyState()
{
    SCOPED_AUTORELEASE_POOL();
    
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
        
    return FModifierKeyState(Mask);
}
