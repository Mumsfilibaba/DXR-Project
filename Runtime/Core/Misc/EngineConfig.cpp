#include "EngineConfig.h"

#include "Core/Misc/OutputDeviceLogger.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConfigFile

FConfigFile::FConfigFile(const CHAR* InFilename)
    : Filename(InFilename)
    , Sections()
{ }

bool FConfigFile::SetString(const CHAR* SectionName, const CHAR* Name, const FString& NewValue)
{
    FConfigValue* Value = GetValue(SectionName, Name);
    if (Value)
    {
        Value->CurrentValue = NewValue;
        return true;
    }
    else
    {
        LOG_ERROR("Failed to set config value '%s' in section '%s'", Name, SectionName);
        return false;
    }
}

bool FConfigFile::SetInt(const CHAR* SectionName, const CHAR* Name, int32 NewValue)
{
    return SetString(SectionName, Name, ToString(NewValue));
}

bool FConfigFile::SetFloat(const CHAR* SectionName, const CHAR* Name, float NewValue)
{
    return SetString(SectionName, Name, ToString(NewValue));
}

bool FConfigFile::SetBoolean(const CHAR* SectionName, const CHAR* Name, bool bNewValue)
{
    return SetString(SectionName, Name, ToString(bNewValue));
}

bool FConfigFile::GetString(const CHAR* SectionName, const CHAR* Name, FString& OutValue)
{
    FConfigValue* Value = GetValue(SectionName, Name);
    if (Value)
    {
        OutValue = Value->CurrentValue;
        return true;
    }
    else
    {
        return false;
    }
}

bool FConfigFile::GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue)
{
    return GetTypedValue(SectionName, Name, OutValue);
}

bool FConfigFile::GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue)
{
    return GetTypedValue(SectionName, Name, OutValue);
}

bool FConfigFile::GetBoolean(const CHAR* SectionName, const CHAR* Name, bool& bOutValue)
{
    return GetTypedValue(SectionName, Name, bOutValue);
}

bool FConfigFile::ParseFile()
{
    return false;
}

bool FConfigFile::SaveFile()
{
    //    FILE* File = fopen( Filename.GetCString(), "w" );
    //    if ( File )
    //    {
    //        for ( auto& CurrentSectionPair : Sections )
    //        {
    //            fprintf( File, "[%s]", CurrentSectionPair.first.GetCString() );
    //
    //            FConfigSection& CurrentSection = CurrentSectionPair.second;
    //            for ( auto& ValuePair : CurrentSection.ConfigValues )
    //            {
    //                FConfigValue& Value = ValuePair.second;
    //                fprintf( File, "%s=%s", ValuePair.first.GetCString(), Value.CurrentValue.GetCString() );
    //
    //                Value.MakeCurrentSaved();
    //            }
    //        }
    //
    //        fclose( File );
    //        return true;
    //    }
    //    else
    //    {
    //        return false;
    //    }
    return false;
}

void FConfigFile::Append(const FConfigFile& OtherFile)
{
    //    for ( auto& OtherSection : OtherFile.Sections )
    //    {
    //        FConfigSection& Section = Sections[OtherSection.first];
    //    }
}

FConfigValue* FConfigFile::GetValue(const CHAR* Name)
{
    //    for ( auto& CurrentSection : Sections )
    //    {
    //        FConfigSection& ConfigSection = CurrentSection.second;
    //        return ConfigSection.GetValue( Name );
    //    }

    return nullptr;
}

FConfigValue* FConfigFile::GetValue(const CHAR* SectionName, const CHAR* Name)
{
    //    FConfigSection* Section = GetSection( SectionName );
    //    if ( Section )
    //    {
    //        return Section->GetValue( Name );
    //    }

    return nullptr;
}

FConfigSection* FConfigFile::GetSection(const CHAR* SectionName)
{
    //    auto Section = Sections.find( SectionName );
    //    if ( Section != Sections.end() )
    //    {
    //        return &Section->second;
    //    }

    return nullptr;
}
