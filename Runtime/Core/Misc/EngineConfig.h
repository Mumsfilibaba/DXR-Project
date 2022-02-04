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

    CConfigValue& operator=(CConfigValue&& Rhs) = default;
    CConfigValue& operator=(const CConfigValue& Rhs) = default;

    /* Create value from string */
    explicit CConfigValue(String&& InString)
        : SavedValue(InString)
        , CurrentValue(Move(InString))
    {
    }

    /* Create value from string */
    explicit CConfigValue(const String& InString)
        : SavedValue(InString)
        , CurrentValue(InString)
    {
    }

    /* Restores the value to the value currently in the save file */
    FORCEINLINE void Restore() { CurrentValue = SavedValue; }

    /* Makes the current value the saved value */
    FORCEINLINE void MakeCurrentSaved() { SavedValue = CurrentValue; }

    /* Current value in the config file */
    String SavedValue;
    /* Current value in the runtime, this will be saved when the file is flushed to disk */
    String CurrentValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A section of a config-file

class CORE_API CConfigSection
{
    friend class CConfigFile;

public:

    CConfigSection(const char* Name);
    ~CConfigSection() = default;

    /* Restores all values in the section */
    void Restore();

    /* Restores a specific value in the section */
    bool Restore(const char* Name);

    /* Set a specific value to a new string */
    void SetValue(const char* Name, const String& NewValue);

    /* Retrieve a value */
    CConfigValue* GetValue(const char* Name);

    /* Retrieve a value */
    const CConfigValue* GetValue(const char* Name) const;

private:
    THashTable<String, CConfigValue, SStringHasher> ConfigValues;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Represent an engine-configuration file

class CORE_API CConfigFile
{
public:

    CConfigFile(const char* Filename);
    ~CConfigFile() = default;

    /* Set a string from the Engine config */
    bool SetString(const char* SectionName, const char* Name, const String& NewValue);

    /* Set a int from the Engine config */
    bool SetInt(const char* SectionName, const char* Name, int32 NewValue);

    /* Set a float from the Engine config */
    bool SetFloat(const char* SectionName, const char* Name, float NewValue);

    /* Set a boolean from the Engine config */
    bool SetBoolean(const char* SectionName, const char* Name, bool bNewValue);

    /* Retrieve a string from the Engine config */
    bool GetString(const char* SectionName, const char* Name, String& OutValue);

    /* Retrieve a int from the Engine config */
    bool GetInt(const char* SectionName, const char* Name, int32& OutValue);

    /* Retrieve a float from the Engine config */
    bool GetFloat(const char* SectionName, const char* Name, float& OutValue);

    /* Retrieve a boolean from the Engine config */
    bool GetBoolean(const char* SectionName, const char* Name, bool& bOutValue);

    /* Loads the file and parses its contents */
    bool ParseFile();

    /* Saves the content to the file */
    bool SaveFile();

    /* Appends all the sections into this config file */
    void Append(const CConfigFile& OtherFile);

private:

    /* Retrieve a section with a certain name */
    CConfigSection* GetSection(const char* SectionName);

    /* Retrieve a variable just based on name */
    CConfigValue* GetValue(const char* Name);

    /* Retrieve a variable from a certain section, with a certain name */
    CConfigValue* GetValue(const char* SectionName, const char* Name);

    /* Templated version of get value */
    template<typename T>
    bool GetTypedValue(const char* SectionName, const char* Name, T& OutValue)
    {
        String Value;
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
    String Filename;

    // All the sections
    THashTable<String, CConfigSection, SStringHasher> Sections;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Main-Engine configuration

extern CORE_API CConfigFile GEngineConfig;