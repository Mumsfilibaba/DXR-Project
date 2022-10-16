#pragma once
#include "Engine/Engine.h"

#define MAX_PROJECT_NAME_LENGTH (512)
#define MAX_PROJECT_PATH_LENGTH (512)
#define MAX_ASSET_PATH_LENGTH   (512)

class ENGINE_API FProjectManager
{
public:
    static bool Initialize(const CHAR* ProjectName, const CHAR* ProjectPath, const CHAR* AssetPath);

    static FORCEINLINE const CHAR* GetProjectName()
    {
        return ProjectName;
    }

    static FORCEINLINE const CHAR* GetProjectModuleName()
    {
        return ProjectName;
    }

    static FORCEINLINE const CHAR* GetProjectPath()
    {
        return ProjectPath;
    }
    
    static FORCEINLINE const CHAR* GetAssetPath()
    {
        return AssetPath;
    }

private:
    static CHAR ProjectName[MAX_PROJECT_NAME_LENGTH];
    static CHAR ProjectPath[MAX_PROJECT_PATH_LENGTH];
    static CHAR AssetPath[MAX_ASSET_PATH_LENGTH];
};
