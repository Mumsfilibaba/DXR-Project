#include "Core/Mac/MacPlatformMisc.h"
#include <sys/sysctl.h>
#include <Foundation/Foundation.h>

bool FMacPlatformMisc::IsDebuggerPresent()
{
    // See: https://developer.apple.com/library/archive/qa/qa1361/_index.html for original implementation
    int32 Mib[4]
    {
        CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()
    };

    struct kinfo_proc Info;
    Info.kp_proc.p_flag = 0;

    size_t Size = sizeof(Info);
    const int32 Junk = sysctl(Mib, sizeof(Mib) / sizeof(*Mib), &Info, &Size, nullptr, 0);
    CHECK(Junk == 0);

    return (Info.kp_proc.p_flag & P_TRACED) != 0;
}

EAssertDialogResult FMacPlatformMisc::ShowAssertDialog(const CHAR* Title, const CHAR* Message)
{
    // CFUserNotification rather than NSAlert: asserts fire from worker threads
    // and from before an NSApplication exists, neither of which NSAlert supports.
    CFStringRef TitleRef   = CFStringCreateWithCString(nullptr, Title,   kCFStringEncodingUTF8);
    CFStringRef MessageRef = CFStringCreateWithCString(nullptr, Message, kCFStringEncodingUTF8);

    CFOptionFlags Response = 0;
    const SInt32 Error = CFUserNotificationDisplayAlert(0.0, kCFUserNotificationStopAlertLevel, nullptr, nullptr, nullptr, 
        TitleRef, MessageRef, CFSTR("Abort"), CFSTR("Debug"), CFSTR("Ignore"), &Response);
    if (TitleRef)
    {
        CFRelease(TitleRef);
    }

    if (MessageRef)
    {
        CFRelease(MessageRef);
    }

    if (Error != 0)
    {
        return EAssertDialogResult::Abort;
    }

    switch (Response & 0x3)
    {
        case kCFUserNotificationDefaultResponse:
            return EAssertDialogResult::Abort;

        case kCFUserNotificationAlternateResponse:
            return EAssertDialogResult::Debug;

        case kCFUserNotificationOtherResponse:
            return EAssertDialogResult::Ignore;

        default:
            return EAssertDialogResult::Abort;
    }
}
