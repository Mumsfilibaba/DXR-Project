#include "ProjectManager.h"

#include "Core/Templates/StringUtils.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CProjectManager

char CProjectManager::ProjectName[MAX_PROJECT_NAME_LENGTH];
char CProjectManager::ProjectPath[MAX_PROJECT_PATH_LENGTH];

bool CProjectManager::Initialize(const char* InProjectName, const char* InProjectPath)
{
    CStringUtils::Copy(CMemory::Memzero<char>(ProjectName, sizeof(ProjectName)), InProjectName);
    CStringUtils::Copy(CMemory::Memzero<char>(ProjectPath, sizeof(ProjectPath)), InProjectPath);
    return true;
}