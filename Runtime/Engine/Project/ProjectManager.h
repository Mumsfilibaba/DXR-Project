#pragma once
#include "Engine/Engine.h"

#define MAX_PROJECT_NAME_LENGTH (512)
#define MAX_PROJECT_PATH_LENGTH (512)
#define MAX_ASSET_PATH_LENGTH   (512)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FProjectManager

class ENGINE_API FProjectManager
{
public:

    static bool Initialize(const char* ProjectName, const char* ProjectPath, const char* AssetPath);

    static FORCEINLINE const char* GetProjectName()
    {
        return ProjectName;
    }

    static FORCEINLINE const char* GetProjectModuleName()
    {
        return ProjectName;
    }

    static FORCEINLINE const char* GetProjectPath()
    {
        return ProjectPath;
    }
    
    static FORCEINLINE const char* GetAssetPath()
    {
        return AssetPath;
    }

private:
    static char ProjectName[MAX_PROJECT_NAME_LENGTH];
    static char ProjectPath[MAX_PROJECT_PATH_LENGTH];
    static char AssetPath[MAX_ASSET_PATH_LENGTH];
};
