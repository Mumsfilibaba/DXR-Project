#pragma once
#include "Core/Containers/String.h"

class FJsonArchive;

struct CORE_API FEngineDescriptor
{
    /** @brief The Version written to the file, and the newest one this engine reads */
    static constexpr int32 CurrentVersion = 1;

    /**
     * @brief Load the descriptor from Filename, replacing everything this one holds. Nothing is replaced if loading fails.
     * @param Filename The dxrengine.json to read
     * @param OutError Receives why loading failed, such as a parse error with its line and column or a field with the wrong type
     * @return Returns false if the file is missing, is not valid JSON, has no Version, was written by a newer engine, has no
     * Name, or holds a field of the wrong type
     */
    bool LoadFromFile(const FString& Filename, FString& OutError);

    /**
     * @brief Save the descriptor to Filename as text JSON, with the Version first
     * @param Filename The file to write, which is replaced when it exists
     * @return Returns false if the file could not be written
     */
    bool SaveToFile(const FString& Filename) const;

    /**
     * @brief Read or write the descriptor's fields through Archive, depending on whether it is loading or saving
     * @param Archive The archive to serialize with, which sets the Version when saving and collects any field errors when loading
     */
    void Serialize(FJsonArchive& Archive);

    /** @brief The name projects store as their EngineAssociation, the same on every machine */
    FString Name;
};
