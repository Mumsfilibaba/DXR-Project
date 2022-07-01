#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/HashTable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Holds a single config-value

class CConfigValue
{
public:

    CConfigValue() = default;
    CConfigValue(CConfigValue&& Other) = default;
    CConfigValue(const CConfigValue& Other) = default;
    ~CConfigValue() = default;

    CConfigValue& operator=(CConfigValue&& RHS) = default;
    CConfigValue& operator=(const CConfigValue& RHS) = default;

     /** @brief: Create value from string */
    explicit CConfigValue(FString&& InString)
        : SavedValue(InString)
        , CurrentValue(Move(InString))
    { }

     /** @brief: Create value from string */
    explicit CConfigValue(const FString& InString)
        : SavedValue(InString)
        , CurrentValue(InString)
    { }

     /** @brief: Restores the value to the value currently in the save file */
    FORCEINLINE void Restore() { CurrentValue = SavedValue; }

     /** @brief: Makes the current value the saved value */
    FORCEINLINE void MakeCurrentSaved() { SavedValue = CurrentValue; }

     /** @brief: Current value in the config file */
    FString SavedValue;
     /** @brief: Current value in the runtime, this will be saved when the file is flushed to disk */
    FString CurrentValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A section of a config-file

class CORE_API CConfigSection
{
    friend class CConfigFile;

public:

    CConfigSection(const char* Name);
    ~CConfigSection() = default;

     /** @brief: Restores all values in the section */
    void Restore();

     /** @brief: Restores a specific value in the section */
    bool Restore(const char* Name);

     /** @brief: Set a specific value to a new string */
    void SetValue(const char* Name, const FString& NewValue);

     /** @brief: Retrieve a value */
    CConfigValue* GetValue(const char* Name);

     /** @brief: Retrieve a value */
    const CConfigValue* GetValue(const char* Name) const;

private:
    THashTable<FString, CConfigValue, FStringHasher> ConfigValues;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Represent an engine-configuration file

class CORE_API CConfigFile
{
public:

    CConfigFile(const char* Filename);
    ~CConfigFile() = default;

     /** @brief: Set a string from the Engine config */
    bool SetString(const char* SectionName, const char* Name, const FString& NewValue);

     /** @brief: Set a int from the Engine config */
    bool SetInt(const char* SectionName, const char* Name, int32 NewValue);

     /** @brief: Set a float from the Engine config */
    bool SetFloat(const char* SectionName, const char* Name, float NewValue);

     /** @brief: Set a boolean from the Engine config */
    bool SetBoolean(const char* SectionName, const char* Name, bool bNewValue);

     /** @brief: Retrieve a string from the Engine config */
    bool GetString(const char* SectionName, const char* Name, FString& OutValue);

     /** @brief: Retrieve a int from the Engine config */
    bool GetInt(const char* SectionName, const char* Name, int32& OutValue);

     /** @brief: Retrieve a float from the Engine config */
    bool GetFloat(const char* SectionName, const char* Name, float& OutValue);

     /** @brief: Retrieve a boolean from the Engine config */
    bool GetBoolean(const char* SectionName, const char* Name, bool& bOutValue);

     /** @brief: Loads the file and parses its contents */
    bool ParseFile();

     /** @brief: Saves the content to the file */
    bool SaveFile();

     /** @brief: Appends all the sections into this config file */
    void Append(const CConfigFile& OtherFile);

private:

     /** @brief: Retrieve a section with a certain name */
    CConfigSection* GetSection(const char* SectionName);

     /** @brief: Retrieve a variable just based on name */
    CConfigValue* GetValue(const char* Name);

     /** @brief: Retrieve a variable from a certain section, with a certain name */
    CConfigValue* GetValue(const char* SectionName, const char* Name);

     /** @brief: Templated version of get value */
    template<typename T>
    bool GetTypedValue(const char* SectionName, const char* Name, T& OutValue)
    {
        FString Value;
        if (GetString(SectionName, Name, Value))
        {
            return FromString<T>(Value, OutValue);
        }
        else
        {
            return false;
        }
    }

    // The filename
    FString Filename;

    // All the sections
    THashTable<FString, CConfigSection, FStringHasher> Sections;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Main-Engine configuration

extern CORE_API CConfigFile GEngineConfig;