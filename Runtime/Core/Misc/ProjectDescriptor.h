#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

class FJsonArchive;

enum class EProjectModuleType : uint8
{
    /** @brief Loaded by the game and by the editor */
    Game,

    /** @brief Loaded only by editor builds */
    Editor,
};

struct CORE_API FProjectModuleDescriptor
{
    void Serialize(FJsonArchive& Archive);

    String             Name;
    EProjectModuleType Type = EProjectModuleType::Game;
};

struct CORE_API FProjectDescriptor
{
    static constexpr int32 CurrentVersion = 1;

    static constexpr const CHAR* FileExtension = ".dxrproject";

    /**
     * @brief Load the descriptor from Filename, replacing everything this one holds. Nothing is replaced if loading fails.
     * @param OutError Receives why loading failed, such as a parse error with its line and column or a field with the wrong type
     * @return Returns false if the file is missing, is not valid JSON, has no Version, was written by a newer engine, has no
     * Name, or holds a field of the wrong type
     */
    bool LoadFromFile(const String& Filename, String& OutError);

    /** @return Returns false if the file could not be written */
    bool SaveToFile(const String& Filename) const;

    void Serialize(FJsonArchive& Archive);

    String Name;

    /** @brief The ID of the engine install the project uses, looked up in the per-user engine registry */
    String EngineAssociation;

    /** @brief A path to the engine, relative to the descriptor or absolute, used when EngineAssociation does not resolve */
    String EnginePath;

    TArray<FProjectModuleDescriptor> Modules;
};
