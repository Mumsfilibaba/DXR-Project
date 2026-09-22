#pragma once
#include "Core/Misc/IniFile.h"

// Declaration order is priority order, every layer overrides the ones declared before it
struct EConfigFile
{
    enum Type : uint8
    {
        /** Base layer, read by every instance of the engine */
        Engine = 0,

        /** Read whenever a game runs, including the one inside the editor */
        Game,

        /** Editor-only, overrides the game and engine layers */
        Editor,

        /** Named after the platform, and has the last word over every other layer */
        Platform,

        Count,
    };
};

class CORE_API FConfig
{
public:
    static bool Initialize();
    static void Release();

public:

    /** @brief Loads a file and keeps it open, without making it part of any layer */
    FIniFile* LoadFile(const String& Filename);

    /** @return Returns the file installed as 'ConfigFile', or nullptr when that layer is absent */
    FIniFile* GetFile(EConfigFile::Type ConfigFile);

    /** @brief Installs 'File' as 'ConfigFile', the caller keeps ownership of the file */
    void SetFile(EConfigFile::Type ConfigFile, FIniFile* File);

    /** @return Looks up a value in the highest layer that has it, and returns nullptr if no layer does */
    FIniValue* FindValue(const CHAR* SectionName, const CHAR* Name);

    /** @brief Retrieve a string from the highest layer that has it */
    bool GetString(const CHAR* SectionName, const CHAR* Name, String& OutValue);

    /** @brief Retrieve a int from the highest layer that has it */
    bool GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue);

    /** @brief Retrieve a float from the highest layer that has it */
    bool GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue);

    /** @brief Retrieve a boolean from the highest layer that has it */
    bool GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue);

    /** @brief Applies every layer to the console variables, lowest layer first so that the highest one wins */
    void LoadConsoleVariables();

private:
    FConfig();
    ~FConfig();

    FIniFile* AddLayer(EConfigFile::Type ConfigFile, const String& Filename);
    void AddDefaultLayers();

    TMap<String, FIniFile> ConfigFiles;
    FIniFile*              LayerFiles[EConfigFile::Count];
};

extern CORE_API FConfig* GConfig;
