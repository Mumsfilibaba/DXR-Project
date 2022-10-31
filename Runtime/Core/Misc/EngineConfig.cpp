#include "EngineConfig.h"
#include "Parse.h"

#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"

FConfigFile GConfig(ENGINE_LOCATION"/Engine.ini");

FConfigSection::FConfigSection()
    : Name()
    , Values()
{ }

FConfigSection::FConfigSection(const CHAR* InName)
    : Name(InName)
    , Values()
{ }

bool FConfigSection::AddValue(const CHAR* NewKey, const CHAR* NewValue)
{
    auto Value = Values.find(FString(NewValue));
    if (Value != Values.end())
    {
        return false;
    }

    const auto Result = Values.insert(std::make_pair(NewKey, NewValue));
    return Result.second;
}

FConfigValue* FConfigSection::FindOrAddValue(const CHAR* InKey, const CHAR* InValue)
{
    auto Value = Values.find(FString(InValue));
    if (Value != Values.end())
    {
        return &Value->second;
    }

    auto Result = Values.insert(std::make_pair(InKey, InValue));
    if (Result.second)
    {
        return &Result.first->second;
    }

    return nullptr;
}

FConfigValue* FConfigSection::FindValue(const CHAR* Key)
{
    auto Value = Values.find(FString(Key));
    if (Value != Values.end())
    {
        return &Value->second;
    }

    return nullptr;
}

const FConfigValue* FConfigSection::FindValue(const CHAR* Key) const
{
    auto Value = Values.find(FString(Key));
    if (Value != Values.end())
    {
        return &Value->second;
    }

    return nullptr;
}

void FConfigSection::Restore()
{
    for (auto Value : Values)
    {
        Value.second.Restore();
    }
}


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

    // Create an empty Section
    FConfigSection* CurrentSection = nullptr;

    CHAR* Start = FileContents.GetData();
    while (Start && (*Start != '\0'))
    {
        // Skip newline chars
        while (*Start == '\n')
            ++Start;

        CHAR* LineStart = Start;
        FParse::ParseLine(&Start);

        // End string at the end of line
        if (*Start == '\n')
        {
            *(Start++) = '\0';
        }

        // Skip any spaces at the beginning of the line
        FParse::ParseWhiteSpace(&LineStart);
        
        // This is a section
        if (*LineStart == '[')
        {
            if (CHAR* SectionEnd = FCString::Strchr(++LineStart, ']'))
            {
                CHAR* SectionStart = LineStart;
                *SectionEnd = '\0';
                CurrentSection = FindOrAddSection(SectionStart);
            }
        }
        else if (*LineStart != ';') // Check if this is a comment line
        {
            if (CHAR* EqualSign = FCString::Strchr(LineStart, '='))
            {
                CHAR* KeyEnd = EqualSign - 1;
                while (*KeyEnd == ' ')
                    *(KeyEnd--) = '\0';

                // The parsed key
                CHAR* Key = LineStart;
                LineStart = EqualSign + 1;
                
                FParse::ParseWhiteSpace(&LineStart);

                // Find the end of the value
                CHAR* Value    = LineStart;
                CHAR* ValueEnd = nullptr;

                // Special case for string-values
                if (*Value == '\"')
                {
                    Value++;
                    ValueEnd = FCString::Strchr(Value, '\"');
                    *ValueEnd = '\0';
                }
                else
                {
                    ValueEnd = FCString::Strchr(LineStart, ' ');
                }

                // Use line-end as backup
                if (!ValueEnd)
                {
                    ValueEnd = Start;
                }

                while (*ValueEnd == ' ')
                    *(ValueEnd--) = '\0';

                // If there are no section, use the global one
                if (!CurrentSection)
                {
                    CurrentSection = FindOrAddSection("");
                }

                // The parsed value
                if (FConfigValue* CurrentValue = CurrentSection->FindValue(Key))
                {
                    *CurrentValue = ::Move(FConfigValue(Value));
                }
                else
                {
                    CurrentSection->AddValue(Key, Value);
                }
            }
        }
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
    FConfigValue* Value = FindValue(SectionName, Name);
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
    if (FConfigValue* Value = FindValue(SectionName, Name))
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

FConfigValue* FConfigFile::FindValue(const CHAR* Key)
{
    for (auto& CurrentSection : Sections)
    {
        FConfigSection& ConfigSection = CurrentSection.second;
        return ConfigSection.FindValue(Key);
    }

    return nullptr;
}

FConfigValue* FConfigFile::FindValue(const CHAR* SectionName, const CHAR* Name)
{
    if (FConfigSection* Section = FindSection(SectionName))
    {
        return Section->FindValue( Name );
    }

    return nullptr;
}

FConfigSection* FConfigFile::FindSection(const CHAR* SectionName)
{
    auto Section = Sections.find(SectionName);
    if (Section != Sections.end())
    {
        return &Section->second;
    }

    return nullptr;
}

FConfigSection* FConfigFile::FindOrAddSection(const CHAR* SectionName)
{
    const FString TempName = SectionName;

    auto Section = Sections.find(TempName);
    if (Section != Sections.end())
    {
        return &Section->second;
    }

    auto Result = Sections.insert(std::make_pair(SectionName, FConfigSection(SectionName)));
    if (Result.second)
    {
        return &Result.first->second;
    }

    return nullptr;
}
