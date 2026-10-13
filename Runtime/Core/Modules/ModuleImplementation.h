#pragma once
#include "Core/Modules/ModuleManager.h"
#include "Core/Misc/BuildStampDefines.h"

// Include this only in the file that implements a module with IMPLEMENT_ENGINE_MODULE, since everything that includes it
// recompiles when the build stamp changes

#if DEBUG_BUILD
    #define ENGINE_MODULE_BUILD_STAMP_CONFIGURATION "Debug"
#elif DEVELOPMENT_BUILD
    #define ENGINE_MODULE_BUILD_STAMP_CONFIGURATION "Development"
#elif RELEASE_BUILD
    #define ENGINE_MODULE_BUILD_STAMP_CONFIGURATION "Release"
#else
    #define ENGINE_MODULE_BUILD_STAMP_CONFIGURATION "Unknown"
#endif

#if EDITOR_BUILD
    #define ENGINE_MODULE_BUILD_STAMP_TARGET " Editor"
#else
    #define ENGINE_MODULE_BUILD_STAMP_TARGET ""
#endif

#if BUILD_WORKING_TREE_DIRTY
    #define ENGINE_MODULE_BUILD_STAMP_DIRTY "+dirty"
#else
    #define ENGINE_MODULE_BUILD_STAMP_DIRTY ""
#endif

#if WIDE_CHAR_BUILD
    #define ENGINE_MODULE_BUILD_STAMP_CHARACTERS "Wide"
#else
    #define ENGINE_MODULE_BUILD_STAMP_CHARACTERS "Narrow"
#endif

/**
 * What the modules of a modular build have to agree on to be loaded together: the commit the project files were generated
 * from and whether the source folders had uncommitted changes, the configuration and the character width, such as
 * "ef2c6a7feb41+dirty Development Editor Narrow"
 */
#define ENGINE_MODULE_BUILD_STAMP \
    BUILD_COMMIT ENGINE_MODULE_BUILD_STAMP_DIRTY " " ENGINE_MODULE_BUILD_STAMP_CONFIGURATION ENGINE_MODULE_BUILD_STAMP_TARGET " " ENGINE_MODULE_BUILD_STAMP_CHARACTERS

/**
 * Macro for implementing a new engine module based on monolithic or dynamic build. A dynamic module also exports its
 * build stamp, which FModuleManager reads before loading it.
 */

#if MONOLITHIC_BUILD
#define IMPLEMENT_ENGINE_MODULE(ModuleClassType, ModuleName) \
    /** @brief Self registering object for static modules */ \
    static TStaticModuleInitializer<ModuleClassType> GModuleInitializer( TEXT(#ModuleName) ); \
    /** @brief This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" void LinkModule_##ModuleName() { }
#else
#define IMPLEMENT_ENGINE_MODULE(ModuleClassType, ModuleName) \
    extern "C" \
    { \
        MODULE_EXPORT IModule* LoadEngineModule() \
        { \
            return new ModuleClassType(); \
        } \
        /** @brief Read from the file before the module is loaded, see FModuleManager::BuildStampSymbol */ \
        MODULE_EXPORT extern const ANSICHAR GEngineModuleBuildStamp[] = ENGINE_MODULE_BUILD_STAMP; \
    } \
    /** @brief This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" MODULE_EXPORT void LinkModule_##ModuleName() { } \
    IMPLEMENT_NEW_AND_DELETE_OPERATORS()
#endif
