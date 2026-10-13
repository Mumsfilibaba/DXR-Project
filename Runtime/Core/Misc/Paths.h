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

    /**
     * @brief Find the folder a modular build loads the project's modules from: <ProjectDir>/Build/bin/<Configuration>, where
     * <Configuration> is the folder below the Build/bin that holds the executable, such as "Development Editor-windows-x64"
     * @param ProjectDir The project directory
     * @param ExecutablePath The path of the running executable, which may be inside an application bundle
     * @return Returns the folder, or an empty string when the executable is not in a Build/bin folder
     */
    static FString FindProjectBinariesDir(const FString& ProjectDir, const FString& ExecutablePath);

    /**
     * @brief Find the folder logs, user state and caches are written under: the project directory in development, and a
     * folder named after the project in the per-user settings directory in a packaged layout, which may be read-only
     * @param ProjectDir The project directory
     * @param ProjectName The project name, which names the per-user folder
     * @param bIsPackaged Whether the game runs from a packaged layout
     * @param UserSettingsDir The engine's per-user settings directory, see GetUserSettingsDir
     * @return Returns the folder, or ProjectDir when a packaged game has no per-user settings directory to write to
     */
    static FString FindWritableDir(const FString& ProjectDir, const FString& ProjectName, bool bIsPackaged, const FString& UserSettingsDir);

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

    /**
     * @return Returns the engine's Cache directory, for the caches every project shares, which can be deleted and rebuilt. In
     * a packaged layout it is the project's Cache directory instead, since the package may be read-only.
     */
    static FString GetEngineCacheDir();

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

    /** @return Returns whether the game runs from a packaged layout, where the project was found through Package.ini */
    static bool IsPackaged();

    /** @return Returns the folder the project's Logs, User and Cache directories are in, see FindWritableDir */
    static FString GetWritableDir();

    /** @return Returns the project's Cache directory, for data that can be deleted and rebuilt, such as thumbnails */
    static FString GetProjectCacheDir();

    /** @return Returns the project's Logs directory, for the output log and profiling captures */
    static FString GetProjectLogsDir();

    /** @return Returns the project's User directory, for per-user state such as the editor layout */
    static FString GetProjectUserDir();

    /**
     * @return Returns Path when it is absolute, and otherwise Path under the project's Logs directory. Used for output files
     * that a console variable or command names, such as dumps and screenshots.
     */
    static FString ResolveOutputPath(const FString& Path);

    /**
     * @return Returns the name of the first Game module in the project descriptor, PROJECT_NAME when the project has no
     * descriptor, or an empty string when the descriptor lists no Game module
     */
    static FString GetProjectModuleName();

    /** @return Returns the folder a modular build loads the project's modules from first, see FindProjectBinariesDir */
    static FString GetProjectBinariesDir();

    /**
     * @return Returns the engine's folder in the per-user settings directory, e.g. %LOCALAPPDATA%/DXR-Engine on Windows,
     * or an empty string if the platform's directory could not be found
     */
    static FString GetUserSettingsDir();
};
