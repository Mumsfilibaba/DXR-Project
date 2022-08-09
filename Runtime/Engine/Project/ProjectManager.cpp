#include "ProjectManager.h"

#include "Core/Templates/CStringHelper.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FProjectManager

char FProjectManager::ProjectName[MAX_PROJECT_NAME_LENGTH];
char FProjectManager::ProjectPath[MAX_PROJECT_PATH_LENGTH];
char FProjectManager::AssetPath[MAX_ASSET_PATH_LENGTH];

bool FProjectManager::Initialize(const char* InProjectName, const char* InProjectPath, const char* InAssetPath)
{
    FCString::Copy(reinterpret_cast<char*>(FMemory::Memzero(ProjectName, sizeof(ProjectName))), InProjectName);
    FCString::Copy(reinterpret_cast<char*>(FMemory::Memzero(ProjectPath, sizeof(ProjectPath))), InProjectPath);
    FCString::Copy(reinterpret_cast<char*>(FMemory::Memzero(AssetPath  , sizeof(AssetPath)))  , InAssetPath);
    return true;
}
