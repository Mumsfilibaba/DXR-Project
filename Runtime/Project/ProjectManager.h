#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

class PROJECT_API FProjectManager
{
public:
    static bool Initialize();
    static void Release();

    static FProjectManager& Get();

    FStringView GetProjectName()       { return FStringView(ProjectName); }
    FStringView GetProjectModuleName() { return FStringView(ProjectName); }
    FStringView GetProjectPath()       { return FStringView(ProjectPath); }
    FStringView GetAssetPath()         { return FStringView(AssetPath); }

private:
    FProjectManager(const FString& InProjectName, const FString& InProjectPath, const FString& InAssetPath);
    ~FProjectManager() = default;

    FString ProjectName;
    FString ProjectPath;
    FString AssetPath;

    static FProjectManager* GInstance;
};
