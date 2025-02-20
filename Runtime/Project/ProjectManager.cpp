#include "ProjectManager.h"
#include "Core/Templates/CString.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Misc/OutputDeviceLogger.h"

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Project);

FProjectManager* FProjectManager::Instance = nullptr;

FProjectManager::FProjectManager(const FString& InProjectName, const FString& InProjectPath, const FString& InAssetPath)
    : ProjectName(InProjectName)
    , ProjectPath(InProjectPath)
    , AssetPath(InAssetPath)
{
}

bool FProjectManager::Initialize()
{
    const FString ProjectName = PROJECT_NAME;
    const FString ProjectPath = FString(ENGINE_LOCATION) + "/" + ProjectName;
    if (!FPlatformFile::IsDirectory(*ProjectPath))
    {
        DEBUG_BREAK();
        return false;
    }

    const FString AssetFolderPath = FString(ENGINE_LOCATION) + FString("/Assets");
    if (!FPlatformFile::IsDirectory(*AssetFolderPath))
    {
        DEBUG_BREAK();
        return false;
    }

    if (!Instance)
    {
        Instance = new FProjectManager(ProjectName, ProjectPath, AssetFolderPath);
    }

#if !PRODUCTION_BUILD
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
    LOG_INFO("ProjectName=%s", *FProjectManager::Get().GetProjectName());
    LOG_INFO("ProjectPath=%s", *FProjectManager::Get().GetProjectPath());
#endif

    return true;
}

void FProjectManager::Release()
{
    if (Instance)
    {
        delete Instance;
        Instance = nullptr;
    }
}
