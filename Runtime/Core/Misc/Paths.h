#pragma once
#include "Core/Containers/String.h"

struct CORE_API FPaths
{
    static FString GetEngineDir();

    static FString GetAssetDir();

    static FString GetProjectDir();

    static FString GetProjectName();

    static FString GetProjectModuleName();
};