#include "Core/Mac/MacPlatformSystemClipboard.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include <AppKit/AppKit.h>

bool FMacPlatformSystemClipboard::HasText()
{
    return FMacThreadManager::Get().MainThreadDispatchAndReturn(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
        if (!Pasteboard)
        {
            return false;
        }

        NSString* Str = [Pasteboard stringForType:NSPasteboardTypeString];
        return (Str != nil && [Str length] > 0);
    }, NSDefaultRunLoopMode);
}

bool FMacPlatformSystemClipboard::GetText(String& OutText)
{
    OutText.Clear();

    __block String Text;
    const bool bResult = FMacThreadManager::Get().MainThreadDispatchAndReturn(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
        if (!Pasteboard)
        {
            return false;
        }

        NSString* Str = [Pasteboard stringForType:NSPasteboardTypeString];
        if (!Str || [Str length] == 0)
        {
            return false;
        }

        const CHAR* Utf8 = [Str UTF8String];
        if (!Utf8)
        {
            return false;
        }

        Text = String(Utf8);
        return true;
    }, NSDefaultRunLoopMode);

    if (!bResult)
    {
        return false;
    }

    OutText = Move(Text);
    return true;
}

bool FMacPlatformSystemClipboard::SetText(const String& InText)
{
    __block String Text = InText;
    return FMacThreadManager::Get().MainThreadDispatchAndReturn(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
        if (!Pasteboard)
        {
            return false;
        }

        [Pasteboard clearContents];

        const CHAR* Utf8 = *Text;
        if (!Utf8)
        {
            return false;
        }

        NSString* Str = [NSString stringWithUTF8String:Utf8];
        if (!Str)
        {
            return false;
        }

        return [Pasteboard setString:Str forType:NSPasteboardTypeString] == YES;
    }, NSDefaultRunLoopMode);
}

void FMacPlatformSystemClipboard::Clear()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

        NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
        if (!Pasteboard)
        {
            return;
        }

        [Pasteboard clearContents];
    }, NSDefaultRunLoopMode, true);
}
