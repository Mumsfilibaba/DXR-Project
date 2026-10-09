#pragma once
#include "CoreApplication/PlatformInterface/IPlatformDialogs.h"

struct COREAPPLICATION_API FWindowsPlatformDialogs final : public IPlatformDialogs
{
    static bool OpenFile(const FFileDialogDesc& Desc, TArray<String>& OutFilenames);
    static bool SaveFile(const FFileDialogDesc& Desc, String& OutFilename);
    static bool PickFolder(const FFileDialogDesc& Desc, String& OutDirectory);
};
