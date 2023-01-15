#include "ProjectManager.h"
#include "Core/Templates/CString.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Platform/PlatformFile.h"

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Project);

FProjectManager* FProjectManager::GInstance = nullptr;

FProjectManager::FProjectManager(const FString& InProjectName, const FString& InProjectPath, const FString& InAssetPath)
    : ProjectName(InProjectName)
    , ProjectPath(InProjectPath)
    , AssetPath(InAssetPath)
{ }

bool FProjectManager::Initialize()
{
    const FString TmpProjectName = PROJECT_NAME;

    const FString TmpProjectPath = FString(ENGINE_LOCATION) + FString("/") + FString(TmpProjectName);
    if (!FPlatformFile::DoesDirectoryExist(TmpProjectPath.GetCString()))
    {
        DEBUG_BREAK();
        return false;
    }

    const FString TmpAssetFolderPath = FString(ENGINE_LOCATION) + FString("/Assets");
    if (!FPlatformFile::DoesDirectoryExist(TmpAssetFolderPath.GetCString()))
    {
        DEBUG_BREAK();
        return false;
    }

    if (!GInstance)
    {
        GInstance = new FProjectManager(TmpProjectName, TmpProjectPath, TmpAssetFolderPath);
    }

    return true;
}

void FProjectManager::Release()
{
    if (GInstance)
    {
        delete GInstance;
        GInstance = nullptr;
    }
}

FProjectManager& FProjectManager::Get()
{
    CHECK(GInstance != nullptr);
    return *GInstance;
}
