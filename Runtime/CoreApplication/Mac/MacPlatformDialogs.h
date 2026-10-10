#pragma once
#include "CoreApplication/PlatformInterface/IPlatformDialogs.h"

struct COREAPPLICATION_API FMacPlatformDialogs final : public IPlatformDialogs
{
    static bool OpenFile(const FFileDialogDesc& Desc, TArray<FString>& OutFilenames);
    static bool SaveFile(const FFileDialogDesc& Desc, FString& OutFilename);
    static bool PickFolder(const FFileDialogDesc& Desc, FString& OutDirectory);
};
