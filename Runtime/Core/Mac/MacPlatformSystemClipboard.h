#pragma once
#include "Core/Generic/GenericPlatformSystemClipboard.h"

struct FMacPlatformSystemClipboard : public FGenericPlatformSystemClipboard
{
    static bool HasText();
    static bool GetText(String& OutText);
    static bool SetText(const String& InText);
    static void Clear();
};
