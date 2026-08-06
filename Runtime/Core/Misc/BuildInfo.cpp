#include "Core/CoreDefines.h"
#include "Core/Misc/BuildInfo.h"

#if __has_include("Core/Generated/BuildStamp.h")
    #include "Core/Generated/BuildStamp.h"
#endif

#ifndef ENGINE_VERSION_STRING
    #define ENGINE_NAME              "DXR-Engine"
    #define ENGINE_VERSION_STRING    "0.0.0"
    #define BUILD_BRANCH             "Unknown"
    #define BUILD_COMMIT             "Unknown"
    #define BUILD_COMMIT_DATE        "Unknown"
    #define BUILD_WORKING_TREE_DIRTY (0)
#endif

#define BUILDINFO_STRINGIFY_IMPL(Value) #Value
#define BUILDINFO_STRINGIFY(Value)      BUILDINFO_STRINGIFY_IMPL(Value)

const CHAR* BuildInfo::GetEngineName()
{
    return ENGINE_NAME;
}

const CHAR* BuildInfo::GetVersionString()
{
    return ENGINE_VERSION_STRING;
}

const CHAR* BuildInfo::GetConfigurationName()
{
#if DEBUG_BUILD
    #if EDITOR_BUILD
        return "Debug Editor";
    #else
        return "Debug";
    #endif
#elif DEVELOPMENT_BUILD
    #if EDITOR_BUILD
        return "Development Editor";
    #else
        return "Development";
    #endif
#elif RELEASE_BUILD
    #if EDITOR_BUILD
        return "Release Editor";
    #else
        return "Release";
    #endif
#else
    return "Unknown";
#endif
}

const CHAR* BuildInfo::GetLinkageName()
{
#if MONOLITHIC_BUILD
    return "Monolithic";
#else
    return "Modular";
#endif
}

const CHAR* BuildInfo::GetPlatformName()
{
#if PLATFORM_WINDOWS
    return "Windows";
#elif PLATFORM_MACOS
    return "macOS";
#else
    return "Unknown";
#endif
}

const CHAR* BuildInfo::GetArchitectureName()
{
#if PLATFORM_ARCHITECTURE_X86_64
    return "x86_64";
#elif PLATFORM_ARCHITECTURE_ARM64
    return "arm64";
#else
    return "Unknown";
#endif
}

const CHAR* BuildInfo::GetCompilerName()
{
#if PLATFORM_COMPILER_MSVC
    return "MSVC " BUILDINFO_STRINGIFY(_MSC_FULL_VER);
#elif PLATFORM_COMPILER_CLANG
    return "Clang " __clang_version__;
#elif PLATFORM_COMPILER_GCC
    return "GCC " __VERSION__;
#else
    return "Unknown";
#endif
}

const CHAR* BuildInfo::GetCompileTimestamp()
{
    return __DATE__ " " __TIME__;
}

const CHAR* BuildInfo::GetBranch()
{
    return BUILD_BRANCH;
}

const CHAR* BuildInfo::GetCommit()
{
    return BUILD_COMMIT;
}

const CHAR* BuildInfo::GetCommitDate()
{
    return BUILD_COMMIT_DATE;
}

bool BuildInfo::IsWorkingTreeDirty()
{
    return BUILD_WORKING_TREE_DIRTY != 0;
}
