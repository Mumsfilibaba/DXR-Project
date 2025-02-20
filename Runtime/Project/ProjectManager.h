#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

class PROJECT_API FProjectManager
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FProjectManager& Get()
    {
        CHECK(Instance != nullptr);
        return *Instance;
    }

public:
    FStringView GetProjectName() const { return FStringView(ProjectName); }
    FStringView GetProjectModuleName() const { return FStringView(ProjectName); }
    FStringView GetProjectPath() const { return FStringView(ProjectPath); }
    FStringView GetAssetPath() const { return FStringView(AssetPath); }

private:
    FProjectManager(const FString& InProjectName, const FString& InProjectPath, const FString& InAssetPath);
    ~FProjectManager() = default;

    FString ProjectName;
    FString ProjectPath;
    FString AssetPath;

    static FProjectManager* Instance;
};
