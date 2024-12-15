#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "CoreApplication/Mac/MacApplicationMisc.h"
#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

void FMacApplicationMisc::MessageBox(const FString& Title, const FString& Message)
{
    SCOPED_AUTORELEASE_POOL();
    
    CFStringRef CaptionRef = CFStringCreateWithCString(0, *Title,   static_cast<CFStringEncoding>(Title.Length()));
    CFStringRef TextRef    = CFStringCreateWithCString(0, *Message, static_cast<CFStringEncoding>(Message.Length()));
        
    CFOptionFlags Result = 0;
    CFOptionFlags Flags  = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert(0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result);
    
    CFRelease(CaptionRef);
    CFRelease(TextRef);
}

void FMacApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    FMacThreadManager::PumpMessagesAppThread(bUntilEmpty);

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        NSMenu* MainMenu = [NSApp mainMenu];
        [MainMenu update];
    }, NSDefaultRunLoopMode, false);
}
