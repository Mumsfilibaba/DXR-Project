#include "EngineConfig.h"

#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"

FConfigFile GConfig(ENGINE_LOCATION"/Engine.ini");

FConfigFile::FConfigFile(const CHAR* InFilename)
    : Filename(InFilename)
    , Sections()
{ }

bool FConfigFile::ParseFile()
{
    TArray<CHAR> FileContents;
    {
        FFileHandleRef File = FPlatformFile::OpenForRead(Filename);
        if (!File)
        {
            return false;
        }

        // Read the full file
        if (!FFileHelpers::ReadTextFile(File.Get(), FileContents))
        {
            return false;
        }
    }

    // Remove all carriage returns if there are any (Easier to process)
    FileContents.RemoveAll('\r');

    FStringView FileString(FileContents.GetData());
    
    FConfigSection Section;
    while (!FileString.IsEmpty())
    {
        int32 Position = FileString.FindChar('\n');
        if (Position == FStringView::INVALID_INDEX)
        {
            break;
        }

        // Get the current line and remove from the file-string
        FStringView CurrentLine = FileString.SubStringView(0, Position);
        FileString.ShrinkLeftInline(Position + 1);

        Position = CurrentLine.FindChar('=');
        if (Position == FStringView::INVALID_INDEX)
        {
            continue;
        }

        FStringView Key = CurrentLine.SubStringView(0, Position);
        Key.TrimInline();

        FStringView Value = CurrentLine.SubStringView(Position + 1, CurrentLine.GetLength() - Position - 1);
        Value.TrimInline();

        Section.Values.insert(std::make_pair(FString(Key), FString(Value)));
        continue;
    }

    for (const auto& [key, value] : Section.Values)
    {
        LOG_INFO("'%s' = '%s'", key.GetCString(), value.CurrentValue.GetCString());
    }

    return true;
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

bool FConfigFile::SetBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue)
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
    return false;
}

bool FConfigFile::GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue)
{
    return false;
}

bool FConfigFile::GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue)
{
    return false;
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
