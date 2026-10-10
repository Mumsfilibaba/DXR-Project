#pragma once
#include "Core/PlatformInterface/IPlatformSystemClipboard.h"

struct FMacPlatformSystemClipboard : public IPlatformSystemClipboard
{
    static bool HasText();
    static bool GetText(FString& OutText);
    static bool SetText(const FString& InText);
    static void Clear();
};
