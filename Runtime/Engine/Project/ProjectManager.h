#pragma once
#include "Engine/EngineModule.h"

#define MAX_PROJECT_NAME_LENGTH (256)
#define MAX_PROJECT_PATH_LENGTH (512)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// Class responsible for holding information about the project

class ENGINE_API CProjectManager
{
public:

    /* Initialize the project's name */
    static bool Initialize(const char* ProjectName, const char* ProjectPath);

    /* Retrieve the name of the project */
    static FORCEINLINE const char* GetProjectName()
    {
        return ProjectName;
    }

    /* Retrieve the name of the project's module */
    static FORCEINLINE const char* GetProjectModuleName()
    {
        // For now this is the same as the project's name
        return ProjectName;
    }

    /* Retrieve the location of the project's executables */
    static FORCEINLINE const char* GetProjectPath()
    {
        return ProjectPath;
    }

private:
    static char ProjectName[MAX_PROJECT_NAME_LENGTH];
    static char ProjectPath[MAX_PROJECT_PATH_LENGTH];
};
