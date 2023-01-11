#include "ProjectManager.h"
#include "Core/Templates/CString.h"

FProjectManager* FProjectManager::GInstance = nullptr;

FProjectManager::FProjectManager(FStringView InProjectName, FStringView InProjectPath, FStringView InAssetPath)
    : ProjectName(nullptr)
    , ProjectPath(nullptr)
    , AssetPath(nullptr)
{
    ProjectName = FCString::Strcpy(
        reinterpret_cast<CHAR*>(FMemory::MallocZeroed((InProjectName.GetSize() + 1) * sizeof(CHAR))),
        InProjectName.GetCString());
    ProjectPath = FCString::Strcpy(
        reinterpret_cast<CHAR*>(FMemory::MallocZeroed((InProjectPath.GetSize() + 1) * sizeof(CHAR))), 
        InProjectPath.GetCString());
    AssetPath = FCString::Strcpy(
        reinterpret_cast<CHAR*>(FMemory::MallocZeroed((InAssetPath.GetSize() + 1) * sizeof(CHAR))), 
        InAssetPath.GetCString());
}

FProjectManager::~FProjectManager()
{
    SAFE_DELETE(ProjectName);
    SAFE_DELETE(ProjectPath);
    SAFE_DELETE(AssetPath);
}

bool FProjectManager::Initialize(FStringView InProjectName, FStringView InProjectPath, FStringView InAssetPath)
{
    if (!GInstance)
    {
        GInstance = new FProjectManager(InProjectName, InProjectPath, InAssetPath);
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
