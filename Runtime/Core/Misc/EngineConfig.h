#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"

struct FConfigValue
{
    FConfigValue() = default;
    FConfigValue(FConfigValue&& Other) = default;
    FConfigValue(const FConfigValue& Other) = default;
    ~FConfigValue() = default;

    FConfigValue& operator=(FConfigValue&& Other) = default;
    FConfigValue& operator=(const FConfigValue& Other) = default;

     /** @brief - Create value from string */
    explicit FConfigValue(FString&& InString)
        : SavedValue(InString)
        , CurrentValue(Move(InString))
    { }

     /** @brief - Create value from string */
    explicit FConfigValue(const FString& InString)
        : SavedValue(InString)
        , CurrentValue(InString)
    { }

     /** @brief - Restores the value to the value currently in the save file */
    FORCEINLINE void Restore() { CurrentValue = SavedValue; }

     /** @brief - Sets the current value to the saved value */
    FORCEINLINE void SaveCurrent() { SavedValue = CurrentValue; }

     /** @brief - Current value in the config file */
    FString SavedValue;
     /** @brief - Current value in the runtime, this will be saved when the file is flushed to disk */
    FString CurrentValue;
};


struct CORE_API FConfigSection
{
    typedef TMap<FString, FConfigValue, FStringHasher> FConfigValueMap;

    FConfigSection();
    FConfigSection(const CHAR* InName);

     /** @brief - Add a specific value to a new string */
    bool AddValue(const CHAR* NewKey, const CHAR* NewValue);

    /** @brief - Find a value with a certain or add it with the specified value */
    FConfigValue* FindOrAddValue(const CHAR* Key, const CHAR* Value);

     /** @brief - Retrieve a value */
    FConfigValue* FindValue(const CHAR* Key);

     /** @brief - Retrieve a value */
    const FConfigValue* FindValue(const CHAR* Key) const;

    /** @brief - Restores all values in the section */
    void Restore();

    /** @brief - Dump the values to a string */
    void DumpToString(FString& OutString);

private:
    FString         Name;
    FConfigValueMap Values;
};


class CORE_API FConfigFile
{
    typedef TMap<FString, FConfigSection, FStringHasher> FConfigSectionMap;

public:
    FConfigFile(const CHAR* Filename);

     /** @brief - Loads the file and parses its contents */
    bool ParseFile();

     /** @brief - Saves the content to the file */
    bool SaveFile();

     /** @brief - Set a string from the Engine config */
    bool SetString(const CHAR* SectionName, const CHAR* Name, const FString& NewValue);

     /** @brief - Set a int from the Engine config */
    bool SetInt(const CHAR* SectionName, const CHAR* Name, int32 NewValue);

     /** @brief - Set a float from the Engine config */
    bool SetFloat(const CHAR* SectionName, const CHAR* Name, float NewValue);

     /** @brief - Set a boolean from the Engine config */
    bool SetBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue);

     /** @brief - Retrieve a string from the Engine config */
    bool GetString(const CHAR* SectionName, const CHAR* Name, FString& OutValue);

     /** @brief - Retrieve a int from the Engine config */
    bool GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue);

     /** @brief - Retrieve a float from the Engine config */
    bool GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue);

     /** @brief - Retrieve a boolean from the Engine config */
    bool GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue);

private:
    FConfigSection* FindSection(const CHAR* SectionName);
    FConfigSection* FindOrAddSection(const CHAR* SectionName);

    FConfigValue*   FindValue(const CHAR* Name);
    FConfigValue*   FindValue(const CHAR* SectionName, const CHAR* Name);

    FString           Filename;
    FConfigSectionMap Sections;
};

extern CORE_API FConfigFile GConfig;