#pragma once
#include "Core/PlatformInterface/IPlatformSystemClipboard.h"

struct FMacPlatformSystemClipboard : public IPlatformSystemClipboard
{
    static bool HasText();
    static bool GetText(String& OutText);
    static bool SetText(const String& InText);
    static void Clear();
};
