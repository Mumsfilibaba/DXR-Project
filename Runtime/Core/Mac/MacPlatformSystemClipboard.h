#pragma once
#include "Core/Generic/GenericPlatformSystemClipboard.h"

struct FMacPlatformSystemClipboard : public FGenericPlatformSystemClipboard
{
    static bool HasText();
    static bool GetText(FString& OutText);
    static bool SetText(const FString& InText);
    static void Clear();
};
