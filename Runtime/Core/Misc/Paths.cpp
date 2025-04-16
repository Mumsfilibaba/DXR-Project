#include "Core/Misc/Paths.h"

FString FPaths::GetEngineDir()
{
    return FString(ENGINE_LOCATION);
}

FString FPaths::GetAssetDir()
{
    return FPaths::GetEngineDir() + FString("/Assets");
}

FString FPaths::GetProjectDir()
{
    return FString(PROJECT_LOCATION);
}

FString FPaths::GetProjectName()
{
    return FString(PROJECT_NAME);
}

FString FPaths::GetProjectModuleName()
{
    return FString(PROJECT_NAME);
}
