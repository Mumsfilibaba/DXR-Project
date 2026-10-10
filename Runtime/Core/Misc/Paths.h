#pragma once
#include "Core/Containers/String.h"

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

struct CORE_API Paths
{
    /** @brief The file at the root of an engine folder that marks it as one and names the engine */
    static constexpr const CHAR_T* EngineIdentityFilename = "dxrengine.json";

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

    /** @return Returns the project directory baked in at compile time (PROJECT_LOCATION) */
    static FString GetProjectDir();

    /** @return Returns the project name baked in at compile time (PROJECT_NAME) */
    static FString GetProjectName();

    /** @return Returns the name of the project's game module, which is currently the project name */
    static FString GetProjectModuleName();

    /**
     * @return Returns the engine's folder in the per-user settings directory, e.g. %LOCALAPPDATA%/DXR-Engine on Windows,
     * or an empty string if the platform's directory could not be found
     */
    static FString GetUserSettingsDir();
};
