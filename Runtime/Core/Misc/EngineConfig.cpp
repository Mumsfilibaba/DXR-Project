#include "EngineConfig.h"

#include <stdio.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/


/*///////////////////////////////////////////////////////////////////////////////////////////////*/

CConfigFile::CConfigFile( const char* InFilename )
    : Filename( InFilename )
    , Sections()
{
}

bool CConfigFile::SetString( const char* SectionName, const char* Name, const CString& NewValue )
{
    CConfigValue* Value = GetValue( SectionName, Name );
    if ( Value )
    {
        Value->CurrentValue = NewValue;
        return true;
    }
    else
    {
        LOG_ERROR( "Failed to set config value '" + CString( Name ) + "' in section '" + CString( SectionName ) + "'" );
        return false;
    }    
}

bool CConfigFile::SetInt( const char* SectionName, const char* Name, int32 NewValue )
{
    return SetString( SectionName, Name, ToString( NewValue ) );
}

bool CConfigFile::SetFloat( const char* SectionName, const char* Name, float NewValue )
{
    return SetString( SectionName, Name, ToString( NewValue ) );
}

bool CConfigFile::SetBoolean( const char* SectionName, const char* Name, bool bNewValue )
{
    return SetString( SectionName, Name, ToString( bNewValue ) );
}

bool CConfigFile::GetString( const char* SectionName, const char* Name, CString& OutValue )
{
    CConfigValue* Value = GetValue( SectionName, Name );
    if ( Value )
    {
        OutValue = Value->CurrentValue;
        return true;
    }
    else
    {
        return false;
    }    
}

bool CConfigFile::GetInt( const char* SectionName, const char* Name, int32& OutValue )
{
    return GetTypedValue( SectionName, Name, OutValue );
}

bool CConfigFile::GetFloat( const char* SectionName, const char* Name, float& OutValue )
{
    return GetTypedValue( SectionName, Name, OutValue );
}

bool CConfigFile::GetBoolean( const char* SectionName, const char* Name, bool& bOutValue )
{
    return GetTypedValue( SectionName, Name, bOutValue );
}

bool CConfigFile::ParseFile()
{
}

bool CConfigFile::SaveFile()
{
    FILE* File = fopen( Filename.CStr(), "w" );
    if ( File )
    {
        for ( auto& CurrentSectionPair : Sections )
        {
            fprintf( File, "[%s]", CurrentSectionPair.first.CStr() );

            CConfigSection& CurrentSection = CurrentSectionPair.second; 
            for ( auto& ValuePair : CurrentSection.ConfigValues )
            {
                CConfigValue& Value = ValuePair.second;
                fprintf( File, "%s=%s", ValuePair.first.CStr(), Value.CurrentValue );

                Value.MakeCurrentSaved();
            }
        }

        fclose( File );
        return true;
    }
    else
    {
        return false;
    }
}

void CConfigFile::Append( const CConfigFile& OtherFile )
{
    for ( auto& OtherSection : OtherFile.Sections )
    {
        CConfigSection& Section = Sections[OtherSection.first];
    }
}

CConfigValue* CConfigFile::GetValue( const char* Name )
{
    for ( auto& CurrentSection : Sections )
    {
        CConfigSection& ConfigSection = CurrentSection.second;        
        return ConfigSection.GetValue( Name );
    }

    return nullptr;
}

CConfigValue* CConfigFile::GetValue( const char* SectionName, const char* Name )
{
    CConfigSection* Section = GetSection( SectionName );    
    if ( Section )
    {
        return Section->GetValue( Name );
    }

    return nullptr;
}

CConfigSection* CConfigFile::GetSection( const char* SectionName )
{
    auto Section = Sections.find( SectionName );
    if ( Section != Sections.end() )
    {
        return &Section->second;
    }

    return nullptr;
}

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
