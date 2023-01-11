#pragma once
#include "Core/Containers/StringView.h"

class CORE_API FProjectManager
{
    FProjectManager(FStringView InProjectName, FStringView ProjectPath, FStringView AssetPath);
    ~FProjectManager();

public:
    static bool Initialize(FStringView InProjectName, FStringView InProjectPath, FStringView InAssetPath);
    static void Release();

    static FProjectManager& Get();

    FStringView GetProjectName()       { return ProjectName; }
    FStringView GetProjectModuleName() { return ProjectName; }
    FStringView GetProjectPath()       { return ProjectPath; }
    FStringView GetAssetPath()         { return AssetPath; }

private:
    CHAR* ProjectName;
    CHAR* ProjectPath;
    CHAR* AssetPath;

    static FProjectManager* GInstance;
};
