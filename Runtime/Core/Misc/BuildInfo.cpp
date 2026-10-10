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

const CHAR_T* BuildInfo::GetEngineName()
{
    return TEXT(ENGINE_NAME);
}

const CHAR_T* BuildInfo::GetVersionString()
{
    return TEXT(ENGINE_VERSION_STRING);
}

const CHAR_T* BuildInfo::GetConfigurationName()
{
#if DEBUG_BUILD
    #if EDITOR_BUILD
        return TEXT("Debug Editor");
    #else
        return TEXT("Debug");
    #endif
#elif DEVELOPMENT_BUILD
    #if EDITOR_BUILD
        return TEXT("Development Editor");
    #else
        return TEXT("Development");
    #endif
#elif RELEASE_BUILD
    #if EDITOR_BUILD
        return TEXT("Release Editor");
    #else
        return TEXT("Release");
    #endif
#else
    return TEXT("Unknown");
#endif
}

const CHAR_T* BuildInfo::GetLinkageName()
{
#if MONOLITHIC_BUILD
    return TEXT("Monolithic");
#else
    return TEXT("Modular");
#endif
}

const CHAR_T* BuildInfo::GetPlatformName()
{
#if PLATFORM_WINDOWS
    return TEXT("Windows");
#elif PLATFORM_MACOS
    return TEXT("macOS");
#else
    return TEXT("Unknown");
#endif
}

const CHAR_T* BuildInfo::GetArchitectureName()
{
#if PLATFORM_ARCHITECTURE_X86_64
    return TEXT("x86_64");
#elif PLATFORM_ARCHITECTURE_ARM64
    return TEXT("arm64");
#else
    return TEXT("Unknown");
#endif
}

const CHAR_T* BuildInfo::GetCompilerName()
{
#if PLATFORM_COMPILER_MSVC
    return TEXT("MSVC " BUILDINFO_STRINGIFY(_MSC_FULL_VER));
#elif PLATFORM_COMPILER_CLANG
    return TEXT("Clang " __clang_version__);
#elif PLATFORM_COMPILER_GCC
    return TEXT("GCC " __VERSION__);
#else
    return TEXT("Unknown");
#endif
}

const CHAR_T* BuildInfo::GetCompileTimestamp()
{
    return TEXT(__DATE__ " " __TIME__);
}

const CHAR_T* BuildInfo::GetBranch()
{
    return TEXT(BUILD_BRANCH);
}

const CHAR_T* BuildInfo::GetCommit()
{
    return TEXT(BUILD_COMMIT);
}

const CHAR_T* BuildInfo::GetCommitDate()
{
    return TEXT(BUILD_COMMIT_DATE);
}

bool BuildInfo::IsWorkingTreeDirty()
{
    return BUILD_WORKING_TREE_DIRTY != 0;
}
