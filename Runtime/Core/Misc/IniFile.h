#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"

struct FIniValue
{
    FIniValue() = default;
    FIniValue(FIniValue&& Other) = default;
    FIniValue(const FIniValue& Other) = default;
    ~FIniValue() = default;

    FIniValue& operator=(FIniValue&& Other) = default;
    FIniValue& operator=(const FIniValue& Other) = default;

    explicit FIniValue(String&& InString)
        : SavedValue(InString)
        , CurrentValue(::Move(InString))
    {
    }

    explicit FIniValue(const String& InString)
        : SavedValue(InString)
        , CurrentValue(InString)
    {
    }

     /** @brief Restores the value to the value currently in the save file */
    void Restore() 
    { 
        CurrentValue = SavedValue; 
    }

     /** @brief Sets the current value to the saved value */
    void SaveCurrent() 
    {
        SavedValue = CurrentValue;
    }

    bool operator==(const FIniValue& Other) const 
    {
        return SavedValue == Other.SavedValue && CurrentValue == Other.CurrentValue;
    }

    bool operator!=(const FIniValue& Other) const
    {
        return !(*this == Other);
    }

     /** @brief Current value in the config file */
    String SavedValue;

     /** @brief Current value in the runtime, this will be saved when the file is flushed to disk */
    String CurrentValue;
};

struct CORE_API FIniSection
{
    FIniSection();
    FIniSection(const CHAR* InName);

    /** @brief Restores all values in the section */
    void Restore();

    /** @brief Dump the values to a string */
    void DumpToString(String& OutString);

    bool operator==(const FIniSection& Other) const
    {
        return (Name == Other.Name) && (Values == Other.Values);
    }

    bool operator!=(const FIniSection& Other) const
    {
        return !(*this == Other);
    }

    String                  Name;
    TMap<String, FIniValue> Values;
};

struct CORE_API FIniFile
{
    FIniFile()
        : Filename()
        , Sections()
    {
    }

    /** @brief Reads the file at 'InFilename' and parses it, existing values are overwritten */
    bool LoadFromFile(const String& InFilename);

    /** @brief Parses ini-formatted text, note that 'InText' is modified in place while parsing */
    void ParseFromText(TArray<CHAR>& InText);

    /** @return Looks up a value from any section and returns nullptr if not found */
    FIniValue* FindValue(const CHAR* Name);

    /** @return Looks up a value from the section with 'SectionName' and returns nullptr if not found */
    FIniValue* FindValue(const CHAR* SectionName, const CHAR* Name);

     /** @brief Set a string from the Engine config */
    bool SetString(const CHAR* SectionName, const CHAR* Name, const String& NewValue);

     /** @brief Set a int from the Engine config */
    bool SetInt(const CHAR* SectionName, const CHAR* Name, int32 NewValue);

     /** @brief Set a float from the Engine config */
    bool SetFloat(const CHAR* SectionName, const CHAR* Name, float NewValue);

     /** @brief Set a boolean from the Engine config */
    bool SetBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue);

     /** @brief Retrieve a string from the Engine config */
    bool GetString(const CHAR* SectionName, const CHAR* Name, String& OutValue);

     /** @brief Retrieve a int from the Engine config */
    bool GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue);

     /** @brief Retrieve a float from the Engine config */
    bool GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue);

     /** @brief Retrieve a boolean from the Engine config */
    bool GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue);

	/** @brief Saves the content to the file */
	bool WriteToFile();

	/** @brief Prints the content into a string */
	void DumpToString(String& OutString);

    bool operator==(const FIniFile& Other) const
    {
        return Filename == Other.Filename && Sections == Other.Sections;
    }

    bool operator!=(const FIniFile& Other) const
    {
        return !(*this == Other);
    }

    String                    Filename;
    TMap<String, FIniSection> Sections;
};
