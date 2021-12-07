#include "EngineConfig.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/


/*///////////////////////////////////////////////////////////////////////////////////////////////*/


/*///////////////////////////////////////////////////////////////////////////////////////////////*/

TArray<CConfigFile*> CEngineConfig::ConfigFiles;

void CEngineConfig::RegisterConfigFile( CConfigFile* NewConfigFile )
{
    ConfigFiles.Emplace( NewConfigFile );
}

void CEngineConfig::UnregisterConfigFile( CConfigFile* NewConfigFile )
{
    ConfigFiles.Remove( NewConfigFile );
}

bool CEngineConfig::SetString( const char* SectionName, const char* Name, const CString& NewValue )
{
    return false;
}

bool CEngineConfig::SetInt( const char* SectionName, const char* Name, int32 NewValue )
{
    return false;
}

bool CEngineConfig::SetFloat( const char* SectionName, const char* Name, float NewValue )
{
    return false;
}

bool CEngineConfig::SetBoolean( const char* SectionName, const char* Name, bool bNewValue )
{
    return false;
}

bool CEngineConfig::GetString( const char* SectionName, const char* Name, CString& OutValue )
{
    return false;
}

bool CEngineConfig::GetInt( const char* SectionName, const char* Name, int32& OutValue )
{
    return false;
}

bool CEngineConfig::GetFloat( const char* SectionName, const char* Name, float& OutValue )
{
    return false;
}

bool CEngineConfig::GetBoolean( const char* SectionName, const char* Name, bool& bOutValue )
{
    return false;
}
