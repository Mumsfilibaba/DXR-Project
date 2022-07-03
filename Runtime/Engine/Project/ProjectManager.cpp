#include "ProjectManager.h"

#include "Core/Templates/StringUtils.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CProjectManager

char CProjectManager::ProjectName[MAX_PROJECT_NAME_LENGTH];
char CProjectManager::ProjectPath[MAX_PROJECT_PATH_LENGTH];
char CProjectManager::AssetPath[MAX_ASSET_PATH_LENGTH];

bool CProjectManager::Initialize(const char* InProjectName, const char* InProjectPath, const char* InAssetPath)
{
    FStringUtils::Copy(reinterpret_cast<char*>(FMemory::Memzero(ProjectName, sizeof(ProjectName))), InProjectName);
    FStringUtils::Copy(reinterpret_cast<char*>(FMemory::Memzero(ProjectPath, sizeof(ProjectPath))), InProjectPath);
    FStringUtils::Copy(reinterpret_cast<char*>(FMemory::Memzero(AssetPath  , sizeof(AssetPath)))  , InAssetPath);
    
    return true;
}
