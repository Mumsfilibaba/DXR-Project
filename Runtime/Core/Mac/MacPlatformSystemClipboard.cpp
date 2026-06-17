#include "Core/Mac/MacPlatformSystemClipboard.h"
#include "Core/Memory/Memory.h"
#include <AppKit/AppKit.h>

bool FMacPlatformSystemClipboard::HasText()
{
    NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
    if (!Pasteboard)
    {
        return false;
    }

    NSString* Str = [Pasteboard stringForType:NSPasteboardTypeString];
    return (Str != nil && [Str length] > 0);
}

bool FMacPlatformSystemClipboard::GetText(String& OutText)
{
    OutText.Clear();

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

    const WIDECHAR* Utf8 = [Str UTF8String];
    if (!Utf8)
    {
        return false;
    }

    OutText = String(Utf8);
    return true;
}

bool FMacPlatformSystemClipboard::SetText(const String& InText)
{
    NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
    if (!Pasteboard)
    {
        return false;
    }

    [Pasteboard clearContents];

    const WIDECHAR* Utf8 = *InText;
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
}

void FMacPlatformSystemClipboard::Clear()
{
    NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
    if (!Pasteboard)
    {
        return;
    }

    [Pasteboard clearContents];
}
