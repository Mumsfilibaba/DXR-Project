#pragma once
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

    explicit FConfigValue(FString&& InString)
        : SavedValue(InString)
        , CurrentValue(::Move(InString))
    {
    }

    explicit FConfigValue(const FString& InString)
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

    bool operator==(const FConfigValue& Other) const 
    {
        return SavedValue == Other.SavedValue && CurrentValue == Other.CurrentValue;
    }

    bool operator!=(const FConfigValue& Other) const
    {
        return !(*this == Other);
    }

     /** @brief Current value in the config file */
    FString SavedValue;

     /** @brief Current value in the runtime, this will be saved when the file is flushed to disk */
    FString CurrentValue;
};

struct CORE_API FConfigSection
{
    FConfigSection();
    FConfigSection(const CHAR* InName);

    /** @brief Restores all values in the section */
    void Restore();

    /** @brief Dump the values to a string */
    void DumpToString(FString& OutString);

    bool operator==(const FConfigSection& Other) const
    {
        return (Name == Other.Name) && (Values == Other.Values);
    }

    bool operator!=(const FConfigSection& Other) const
    {
        return !(*this == Other);
    }

    FString                     Name;
    TMap<FString, FConfigValue> Values;
};

struct CORE_API FConfigFile
{
    FConfigFile()
        : Filename()
        , Sections()
    {
    }

    /** @return Looks up a value from any section and returns nullptr if not found */
    FConfigValue* FindValue(const CHAR* Name);

    /** @return Looks up a value from the section with 'SectionName' and returns nullptr if not found */
    FConfigValue* FindValue(const CHAR* SectionName, const CHAR* Name);

     /** @brief Set a string from the Engine config */
    bool SetString(const CHAR* SectionName, const CHAR* Name, const FString& NewValue);

     /** @brief Set a int from the Engine config */
    bool SetInt(const CHAR* SectionName, const CHAR* Name, int32 NewValue);

     /** @brief Set a float from the Engine config */
    bool SetFloat(const CHAR* SectionName, const CHAR* Name, float NewValue);

     /** @brief Set a boolean from the Engine config */
    bool SetBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue);

     /** @brief Retrieve a string from the Engine config */
    bool GetString(const CHAR* SectionName, const CHAR* Name, FString& OutValue);

     /** @brief Retrieve a int from the Engine config */
    bool GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue);

     /** @brief Retrieve a float from the Engine config */
    bool GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue);

     /** @brief Retrieve a boolean from the Engine config */
    bool GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue);

	/** @brief Saves the content to the file */
	bool WriteToFile();

	/** @brief Prints the content into a string */
	void DumpToString(FString& OutString);

    bool operator==(const FConfigFile& Other) const
    {
        return Filename == Other.Filename && Sections == Other.Sections;
    }

    bool operator!=(const FConfigFile& Other) const
    {
        return !(*this == Other);
    }

    FString                       Filename;
    TMap<FString, FConfigSection> Sections;
};

extern CORE_API FConfigFile* GConfig;

class CORE_API FConfig
{
public:
    static bool Initialize();
    static void Release();

    FConfigFile* LoadFile(const FString& Filename);

    void LoadConsoleVariables();

private:
    FConfig();

    TMap<FString, FConfigFile> ConfigFiles;
    static FConfig* GlobalConfig;
};
