#pragma once
#include "Core/Containers/String.h"

struct FProjectDescriptor;

enum class EEngineDirSource : uint8
{
    /** @brief No engine directory was found */
    None,

    /** @brief The -engine= command line option */
    CommandLine,

    /** @brief A folder above the running executable */
    ExecutableLocation,

    /** @brief The ENGINE_LOCATION baked in at compile time */
    CompileTimeFallback,
};

enum class EProjectSource : uint8
{
    /** @brief No project descriptor was found, so the project is the PROJECT_LOCATION and PROJECT_NAME baked in at compile time */
    None,

    /** @brief The -project= command line option */
    CommandLine,

    /** @brief The Package.ini next to the executable, in a packaged layout */
    PackageFile,

    /** @brief The descriptor at PROJECT_LOCATION/<PROJECT_NAME>.dxrproject, baked in at compile time */
    CompileTimeFallback,
};

struct CORE_API Paths
{
    /** @brief The file at the root of an engine folder that marks it as one and names the engine */
    static constexpr const CHAR_T* EngineIdentityFilename = TEXT("dxrengine.json");

    /** @brief The file next to the executable of a packaged game, whose [Package] Project names the project relative to it */
    static constexpr const CHAR_T* PackageFilename = TEXT("Package.ini");

    /**
     * @brief Resolve the engine directory from -engine=, the executable's location and the compile-time fallback. Called
     * once the command line is parsed.
     * @return Returns false if none of them is an engine folder, or if -engine= is given without a value
     */
    static bool Initialize();

    /**
     * @brief Find the engine directory, trying EngineOption first, then ExecutableDir and every folder above it, then
     * FallbackDir. A directory is an engine folder when it holds EngineIdentityFilename.
     * @param EngineOption The value of -engine=, or empty when it was not given. When set, it is the only directory tried.
     * @param ExecutableDir The directory of the running executable
     * @param FallbackDir The directory to use when nothing above ExecutableDir is an engine folder
     * @param OutSource Set to where the directory was found, or EEngineDirSource::None on failure. May be nullptr.
     * @return Returns the normalized, absolute engine directory, or an empty string if none was found
     */
    static FString FindEngineDir(
        const FString&    EngineOption,
        const FString&    ExecutableDir,
        const FString&    FallbackDir,
        EEngineDirSource* OutSource = nullptr);

    /**
     * @brief Find and load the project descriptor from -project=, the Package.ini next to the executable, or the compile-time
     * fallback. Called once the engine directory is resolved. Without a descriptor the project is the compile-time one.
     * @param OutError Receives why it failed, such as a -project= that names no project or a descriptor that does not load
     * @return Returns false if -project= or Package.ini names a project that is not there, or if the descriptor found does
     * not load
     */
    static bool InitializeProject(FString& OutError);

    /**
     * @brief Find the project descriptor, trying ProjectOption first, then the Package.ini in ExecutableDir, then FallbackFile.
     * @param ProjectOption The value of -project=, or empty when it was not given: a .dxrproject file, or a folder holding one
     * named after the folder. When set, it is the only place tried.
     * @param ExecutableDir The directory of the running executable. When it holds a Package.ini, that is the only place tried.
     * @param FallbackFile The descriptor to use when neither of the others applies
     * @param OutSource Set to where the descriptor was found, or EProjectSource::None when none was. May be nullptr.
     * @return Returns the normalized, absolute path of the descriptor, or an empty string if none was found
     */
    static FString FindProjectFile(
        const FString&  ProjectOption,
        const FString&  ExecutableDir,
        const FString&  FallbackFile,
        EProjectSource* OutSource = nullptr);

    /** @return Returns where the engine directory was found, resolving it first if Initialize has not run */
    static EEngineDirSource GetEngineDirSource();

    /** @return Returns where the engine directory was found as text for the log, such as "found above the executable" */
    static const CHAR_T* GetEngineDirSourceName();

    /**
     * @return Returns the engine directory, resolving it first if Initialize has not run. Falls back to ENGINE_LOCATION
     * when no engine folder was found.
     */
    static FString GetEngineDir();

    /** @return Returns the engine's Assets directory */
    static FString GetAssetDir();

    /** @return Returns the engine's Content directory, which holds the engine's own assets */
    static FString GetEngineContentDir();

    /** @return Returns the engine's Saved directory, for caches every project shares */
    static FString GetEngineSavedDir();

    /** @return Returns where the project descriptor was found, resolving it first if InitializeProject has not run */
    static EProjectSource GetProjectSource();

    /** @return Returns where the project descriptor was found as text for the log, such as "-project=" */
    static const CHAR_T* GetProjectSourceName();

    /** @return Returns the path of the loaded project descriptor, or an empty string when the project has none */
    static FString GetProjectFile();

    /** @return Returns the loaded project descriptor, or nullptr when the project has none */
    static const FProjectDescriptor* GetProjectDescriptor();

    /** @return Returns the folder holding the project descriptor, or PROJECT_LOCATION when the project has none */
    static FString GetProjectDir();

    /** @return Returns the name in the project descriptor, or PROJECT_NAME when the project has none */
    static FString GetProjectName();

    /** @return Returns the project's Content directory */
    static FString GetProjectContentDir();

    /** @return Returns the project's Config directory */
    static FString GetProjectConfigDir();

    /** @return Returns the project's Saved directory, for logs, layouts and other files that are not checked in */
    static FString GetProjectSavedDir();

    /** @return Returns the name of the project's game module, which is currently the project name */
    static FString GetProjectModuleName();

    /**
     * @return Returns the engine's folder in the per-user settings directory, e.g. %LOCALAPPDATA%/DXR-Engine on Windows,
     * or an empty string if the platform's directory could not be found
     */
    static FString GetUserSettingsDir();
};
