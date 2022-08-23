#include "ProjectManager.h"

#include "Core/Templates/CString.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FProjectManager

CHAR FProjectManager::ProjectName[MAX_PROJECT_NAME_LENGTH];
CHAR FProjectManager::ProjectPath[MAX_PROJECT_PATH_LENGTH];
CHAR FProjectManager::AssetPath[MAX_ASSET_PATH_LENGTH];

bool FProjectManager::Initialize(const CHAR* InProjectName, const CHAR* InProjectPath, const CHAR* InAssetPath)
{
    FCString::Strcpy(reinterpret_cast<CHAR*>(FMemory::Memzero(ProjectName, sizeof(ProjectName))), InProjectName);
    FCString::Strcpy(reinterpret_cast<CHAR*>(FMemory::Memzero(ProjectPath, sizeof(ProjectPath))), InProjectPath);
    FCString::Strcpy(reinterpret_cast<CHAR*>(FMemory::Memzero(AssetPath  , sizeof(AssetPath)))  , InAssetPath);
    return true;
}
