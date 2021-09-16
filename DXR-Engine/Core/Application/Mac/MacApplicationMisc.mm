#if defined(PLATFORM_MACOS) && defined(__OBJC__)
#include "Core/Application/Mac/MacApplicationMisc.h"

#include <Foundation/Foundation.h>

void CMacApplicationMisc::MessageBox( const std::string& Title, const std::string& Message )
{
    CFStringRef CaptionRef  = CFStringCreateWithCString(0, Title.c_str(), Title.size());
    CFStringRef TextRef     = CFStringCreateWithCString(0, Message.c_str(), Message.size());
        
    CFOptionFlags Result    = 0;
    CFOptionFlags Flags     = kCFUserNotificationStopAlertLevel;
    CFUserNotificationDisplayAlert(0, Flags, 0, 0, 0, CaptionRef, TextRef, 0, 0, 0, &Result);
        
    CFRelease(CaptionRef);
    CFRelease(TextRef);
}

#endif
