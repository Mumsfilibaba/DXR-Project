#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

class PROJECT_API FProjectManager
{
    FProjectManager(const FString& InProjectName, const FString& InProjectPath, const FString& InAssetPath);
    ~FProjectManager() = default;

public:
    static bool Initialize();
    static void Release();

    static FProjectManager& Get();

    FStringView GetProjectName()       { return FStringView(ProjectName); }
    FStringView GetProjectModuleName() { return FStringView(ProjectName); }
    FStringView GetProjectPath()       { return FStringView(ProjectPath); }
    FStringView GetAssetPath()         { return FStringView(AssetPath); }

private:
    FString ProjectName;
    FString ProjectPath;
    FString AssetPath;

    static FProjectManager* GInstance;
};
