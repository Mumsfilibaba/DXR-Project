#pragma once
#include "CoreModule.h"

#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/HashTable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CConfigValue
{
public:

    enum
    {
        ValueType_Unknown = 0,
        ValueType_String  = 1,
        ValueType_Int32   = 2,
        ValueType_Int64   = 3,
        ValueType_Float   = 4,
        ValueType_Boolean = 5,
    };

    /* Empty constructor */
    CConfigValue()
        : SavedValue()
        , CurrentValue()
        , Type()
    {
    }
    
    /* Create value from string */
    CConfigValue( CString&& InString )
        : SavedValue( InString )
        , CurrentValue( InString )
        , Type()
    {
    }
    
    /* Create value from string */
    CConfigValue( const CString& InString )
        : SavedValue( InString )
        , CurrentValue( InString )
        , Type()
    {
    }

    /* Move constructor */
    CConfigValue( CConfigValue&& Other )
        : SavedValue( Move( Other.SavedValue ) )
        , CurrentValue( Move( Other.CurrentValue ) )
        , Type( Move( Other.Type ) )
    {
    }
    
    /* Copy constructor */
    CConfigValue( const CConfigValue& Other )
        : SavedValue( Other.SavedValue )
        , CurrentValue( Other.CurrentValue )
        , Type( Other.Type )
    {
    }

    ~CConfigValue() = default;

    /* Retrieves the current value that this variable holds */ 
    FORCEINLINE const CString& GetValue() const { return CurrentValue; }

    /* Retrieves the value that currently exists in the file */
    FORCEINLINE const CString& GetSavedValue() const { return SavedValue; }

    /* Move assignment operator */
    CConfigValue& operator=( CConfigValue&& RHS );
    {

        return *this;
    }

    /* Copy assignment operator */
    CConfigValue& operator=( const CConfigValue& RHS )
    {

        return *this;
    }

private:

    /* Current value in the config file */    
    CString SavedValue;
    /* Current value in the runtime, this will be saved when the file is flushed to disk */
    CString CurrentValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CORE_API CConfigSection
{
public:

    CConfigSection( const char* Name );
    ~CConfigSection() = default;



private:
    THashTable<CString, CConfigValue> ConfigValues;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CORE_API CConfigFile
{
public:

    CConfigFile( const char* Filename );
    ~CConfigFile() = default;

    /* Loads the file and parses its contents */
    void ParseFile();

    /* Saves the content to the file */
    void SaveFile();

    /* Set a string from the Engine config */
    bool SetString( const char* SectionName, const char* Name, const CString& NewValue );

    /* Set a int from the Engine config */
    bool SetInt( const char* SectionName, const char* Name, int32 NewValue );

    /* Set a float from the Engine config */
    bool SetFloat( const char* SectionName, const char* Name, float NewValue );

    /* Set a boolean from the Engine config */
    bool SetBoolean( const char* SectionName, const char* Name, bool bNewValue );

    /* Retrieve a string from the Engine config */
    bool GetString( const char* SectionName, const char* Name, CString& OutValue );

    /* Retrieve a int from the Engine config */
    bool GetInt( const char* SectionName, const char* Name, int32& OutValue );

    /* Retrieve a float from the Engine config */
    bool GetFloat( const char* SectionName, const char* Name, float& OutValue );

    /* Retrieve a boolean from the Engine config */
    bool GetBoolean( const char* SectionName, const char* Name, bool& bOutValue );

private:
    THashTable<CString, CConfigSection, CStringHasher> Sections;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CORE_API CEngineConfig
{
public:

    /* Registers a new Config file to the engine that will be searched when looking for a value */
    static void RegisterConfigFile( CConfigFile* ConfigFile );
    /* Unregister a new Config file from the engine */
    static void UnregisterConfigFile( CConfigFile* ConfigFile );

    /* Set a string from the Engine config */
    static bool SetString( const char* SectionName, const char* Name, const CString& NewValue );

    /* Set a int from the Engine config */
    static bool SetInt( const char* SectionName, const char* Name, int32 NewValue );

    /* Set a float from the Engine config */
    static bool SetFloat( const char* SectionName, const char* Name, float NewValue );

    /* Set a boolean from the Engine config */
    static bool SetBoolean( const char* SectionName, const char* Name, bool bNewValue );

    /* Retrieve a string from the Engine config */
    static bool GetString( const char* SectionName, const char* Name, CString& OutValue );

    /* Retrieve a int from the Engine config */
    static bool GetInt( const char* SectionName, const char* Name, int32& OutValue );

    /* Retrieve a float from the Engine config */
    static bool GetFloat( const char* SectionName, const char* Name, float& OutValue );

    /* Retrieve a boolean from the Engine config */
    static bool GetBoolean( const char* SectionName, const char* Name, bool& bOutValue );

private: 
    static TArray<CConfigFile*> ConfigFiles; 
};