#include "MacApplicationMisc.h"
#include "MacApplication.h"
#include "MacOutputDeviceConsole.h"
#include "Core/Mac/Mac.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Generic/InputCodes.h"

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

FOutputDeviceConsole* FMacApplicationMisc::CreateOutputDeviceConsole()
{
    return new FMacOutputDeviceConsole();
}

void FMacApplicationMisc::MessageBox(const FString& Title, const FString& Message)
{
    SCOPED_AUTORELEASE_POOL();
    
    CFStringRef CaptionRef = CFStringCreateWithCString(0, Title.GetCString(),   static_cast<CFStringEncoding>(Title.Length()));
    CFStringRef TextRef    = CFStringCreateWithCString(0, Message.GetCString(), static_cast<CFStringEncoding>(Message.Length()));
        
    CFOptionFlags Result = 0;
    CFOptionFlags Flags  = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert(0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result);
    
    CFRelease(CaptionRef);
    CFRelease(TextRef);
}

void FMacApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    PumpMessagesAppThread(bUntilEmpty);

    ExecuteOnMainThread(^
    {
        NSMenu* MainMenu = [NSApp mainMenu];
        [MainMenu update];
    }, NSDefaultRunLoopMode, false);
}

FModifierKeyState FMacApplicationMisc::GetModifierKeyState()
{
    SCOPED_AUTORELEASE_POOL();
    
    NSUInteger CurrentModifiers = ([NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask);

    EModifierFlag ModifierFlags = EModifierFlag::None;
    if (CurrentModifiers & NSEventModifierFlagControl)
    {
        ModifierFlags |= EModifierFlag::Ctrl;
    }
    if (CurrentModifiers & NSEventModifierFlagShift)
    {
        ModifierFlags |= EModifierFlag::Shift;
    }
    if (CurrentModifiers & NSEventModifierFlagOption)
    {
        ModifierFlags |= EModifierFlag::Alt;
    }
    if (CurrentModifiers & NSEventModifierFlagCommand)
    {
        ModifierFlags |= EModifierFlag::Super;
    }
    if (CurrentModifiers & NSEventModifierFlagCapsLock)
    {
        ModifierFlags |= EModifierFlag::CapsLock;
    }
    
    return FModifierKeyState(ModifierFlags);
}
