#pragma once
#include "Core/CoreTypes.h"

struct CORE_API BuildInfo
{
    /** @brief Retrieve the engine name */
    static const CHAR_T* GetEngineName();

    /** @brief Retrieve the engine version as "Major.Minor.Patch" */
    static const CHAR_T* GetVersionString();

    /** @brief Retrieve the configuration this module was compiled in, such as "Debug" or "Development Editor" */
    static const CHAR_T* GetConfigurationName();

    /** @brief Retrieve "Monolithic" when every module is linked into the executable, otherwise "Modular" */
    static const CHAR_T* GetLinkageName();

    /** @brief Retrieve the platform this module was compiled for */
    static const CHAR_T* GetPlatformName();

    /** @brief Retrieve the architecture of the running slice, not the one the project files were generated for */
    static const CHAR_T* GetArchitectureName();

    /** @brief Retrieve the compiler and version this module was built with */
    static const CHAR_T* GetCompilerName();

    /** @brief Retrieve when this module was compiled, which in a modular build differs from the executable */
    static const CHAR_T* GetCompileTimestamp();

    /** @brief Retrieve the branch that was checked out when project files were generated */
    static const CHAR_T* GetBranch();

    /** @brief Retrieve the commit that was checked out when project files were generated */
    static const CHAR_T* GetCommit();

    /** @brief Retrieve the ISO 8601 date of that commit */
    static const CHAR_T* GetCommitDate();

    /** @brief Check whether the source folders had uncommitted changes when project files were generated */
    static bool IsWorkingTreeDirty();
};
