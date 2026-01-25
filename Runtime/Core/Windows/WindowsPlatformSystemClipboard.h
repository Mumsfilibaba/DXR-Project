#pragma once
#include "Core/Generic/GenericPlatformSystemClipboard.h"

struct CORE_API FWindowsPlatformSystemClipboard : public FGenericPlatformSystemClipboard
{
    static bool HasText();
    static bool GetText(FString& OutText);
    static bool SetText(const FString& InText);
    static void Clear();
};
