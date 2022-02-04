#pragma once
#include "Engine/Engine.h"

#define MAX_PROJECT_NAME_LENGTH (256)
#define MAX_PROJECT_PATH_LENGTH (512)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CProjectManager - Class responsible for holding information about the project

class ENGINE_API CProjectManager
{
public:

    static bool Initialize(const char* ProjectName, const char* ProjectPath);

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

private:
    static char ProjectName[MAX_PROJECT_NAME_LENGTH];
    static char ProjectPath[MAX_PROJECT_PATH_LENGTH];
};
