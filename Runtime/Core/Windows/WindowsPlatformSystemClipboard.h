#pragma once
#include "Core/PlatformInterface/IPlatformSystemClipboard.h"

struct CORE_API FWindowsPlatformSystemClipboard : public IPlatformSystemClipboard
{
    static bool HasText();
    static bool GetText(FString& OutText);
    static bool SetText(const FString& InText);
    static void Clear();
};
