#include "Core/Misc/Paths.h"

FString Paths::GetEngineDir()
{
    return FString(ENGINE_LOCATION);
}

FString Paths::GetAssetDir()
{
    return Paths::GetEngineDir() + FString("/Assets");
}

FString Paths::GetProjectDir()
{
    return FString(PROJECT_LOCATION);
}

FString Paths::GetProjectName()
{
    return FString(PROJECT_NAME);
}

FString Paths::GetProjectModuleName()
{
    return FString(PROJECT_NAME);
}
