#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

class FJsonArchive;
struct FProjectDescriptor;

struct CORE_API FEngineRegistryEntry
{
    /**
     * @brief Read or write the entry's fields through Archive, depending on whether it is loading or saving
     * @param Archive The archive to serialize with, which collects any field errors when loading
     */
    void Serialize(FJsonArchive& Archive);

    /** @brief The engine's name, as its dxrengine.json gives it */
    FString Name;

    /** @brief The engine folder, absolute and normalized */
    FString Path;
};

struct CORE_API FEngineRegistry
{
    /** @brief The Version written to the file, and the newest one this engine reads */
    static constexpr int32 CurrentVersion = 1;

    /** @brief The name of the registry file in the per-user settings directory */
    static constexpr const CHAR_T* RegistryFilename = TEXT("EngineRegistry.json");

    /**
     * @brief Get where the registry is kept for this user
     * @return Returns the registry file in the per-user settings directory, or an empty string when there is no such directory
     */
    static FString GetDefaultFilename();

    /**
     * @brief Register the engine this editor runs from, under the name in its dxrengine.json, in the default registry file.
     * The file is only written when that changes it.
     * @param OutError Receives why it failed
     * @return Returns false if the engine's descriptor does not load, or the registry file cannot be read or written
     */
    static bool RegisterRunningEngine(FString& OutError);

    /**
     * @brief Load the registry from Filename, replacing everything this one holds. A missing file is an empty registry.
     * @param Filename The registry file to read
     * @param OutError Receives why loading failed, such as a parse error or a newer Version
     * @return Returns false if the file exists but is not a registry this engine can read
     */
    bool LoadFromFile(const FString& Filename, FString& OutError);

    /**
     * @brief Save the registry to Filename as text JSON, with the Version first and the engines in name order
     * @param Filename The file to write, which is replaced when it exists
     * @return Returns false if the file could not be written
     */
    bool SaveToFile(const FString& Filename) const;

    /**
     * @brief Read or write the registry's fields through Archive, depending on whether it is loading or saving
     * @param Archive The archive to serialize with, which sets the Version when saving and collects any field errors when loading
     */
    void Serialize(FJsonArchive& Archive);

    /**
     * @brief Map Name to EngineDir, replacing the folder of an entry with that name
     * @param Name The engine's name, which is compared without case against the names already registered
     * @param EngineDir The engine folder, which is stored absolute and normalized
     * @return Returns true if the registry changed
     */
    bool Register(const FString& Name, const FString& EngineDir);

    /**
     * @brief Look up the folder an engine is registered under
     * @param Name The engine's name, which is compared without case
     * @return Returns the folder registered under Name, or an empty string when no engine has that name
     */
    FString Find(const FString& Name) const;

    /**
     * @brief Find the engine a project uses, for what runs outside an editor: the folder its EngineAssociation is registered
     * under while that is still an engine folder of that name, and otherwise its EnginePath, relative to ProjectDir or absolute
     * @param Project The project's descriptor, whose EngineAssociation and EnginePath are tried in that order
     * @param ProjectDir The folder holding the project's descriptor, which a relative EnginePath starts from
     * @return Returns the engine folder, or an empty string when neither leads to an engine folder
     */
    FString ResolveProjectEngine(const FProjectDescriptor& Project, const FString& ProjectDir) const;

    /** @brief Sorted by name, so the file only changes where an entry does */
    TArray<FEngineRegistryEntry> Engines;
};
