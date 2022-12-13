#include "EngineConfig.h"
#include "Parse.h"
#include "OutputDeviceLogger.h"
#include "ConsoleManager.h"

#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"

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
    if (FConfigValue* CurrentValue = FindValue(NewKey))
    {
        return false;
    }

    const auto Result = Values.emplace(NewKey, NewValue);
    return Result.second;
}

FConfigValue* FConfigSection::FindOrAddValue(const CHAR* InKey, const CHAR* InValue)
{
    if (FConfigValue* CurrentValue = FindValue(InKey))
    {
        return CurrentValue;
    }

    auto Result = Values.emplace(InKey, InValue);
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

void FConfigSection::DumpToString(FString& OutString)
{
    for (auto& ValuePair : Values)
    {
        FConfigValue& Value = ValuePair.second;
        OutString.AppendFormat("%s=%s\n", ValuePair.first.GetCString(), Value.CurrentValue.GetCString());
    }
}



void FConfigFile::Combine(const FConfigFile& ConfigFile)
{
    for (const auto& Section : ConfigFile.Sections)
    {
        Sections.insert(Section);
    }
}

bool FConfigFile::SetString(const CHAR* SectionName, const CHAR* Name, const FString& NewValue)
{
    FConfigValue* Value = nullptr;
    if (SectionName)
    {
        Value = FindValue(SectionName, Name);
    }
    else
    {
        Value = FindValue(Name);
    }

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
    return SetString(SectionName, Name, TTypeToString<int32>::ToString(NewValue));
}

bool FConfigFile::SetFloat(const CHAR* SectionName, const CHAR* Name, float NewValue)
{
    return SetString(SectionName, Name, TTypeToString<float>::ToString(NewValue));
}

bool FConfigFile::SetBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue)
{
    return SetString(SectionName, Name, TTypeToString<bool>::ToString(bNewValue));
}

bool FConfigFile::GetString(const CHAR* SectionName, const CHAR* Name, FString& OutValue)
{
    FConfigValue* Value = nullptr;
    if (SectionName)
    {
        Value = FindValue(SectionName, Name);
    }
    else
    {
        Value = FindValue(Name);
    }

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
    FConfigValue* Value = nullptr;
    if (SectionName)
    {
        Value = FindValue(SectionName, Name);
    }
    else
    {
        Value = FindValue(Name);
    }

    if (Value)
    {
        return TTypeFromString<int32>::FromString(Value->CurrentValue, OutValue);
    }
    else
    {
        return false;
    }
}

bool FConfigFile::GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue)
{
    FConfigValue* Value = nullptr;
    if (SectionName)
    {
        Value = FindValue(SectionName, Name);
    }
    else
    {
        Value = FindValue(Name);
    }

    if (Value)
    {
        return TTypeFromString<float>::FromString(Value->CurrentValue, OutValue);
    }
    else
    {
        return false;
    }
}

bool FConfigFile::GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue)
{
    FConfigValue* Value = nullptr;
    if (SectionName)
    {
        Value = FindValue(SectionName, Name);
    }
    else
    {
        Value = FindValue(Name);
    }

    if (Value)
    {
        return TTypeFromString<bool>::FromString(Value->CurrentValue, bOutValue);
    }
    else
    {
        return false;
    }
}

// NOTE: Why this warning is a thing is a mystery to me
DISABLE_UNREACHABLE_CODE_WARNING

FConfigValue* FConfigFile::FindValue(const CHAR* Key)
{
    for (auto& CurrentSection : Sections)
    {
        FConfigSection& ConfigSection = CurrentSection.second;
        return ConfigSection.FindValue(Key);
    }

    return nullptr;
}

ENABLE_UNREACHABLE_CODE_WARNING

FConfigValue* FConfigFile::FindValue(const CHAR* SectionName, const CHAR* Name)
{
    if (FConfigSection* Section = FindSection(SectionName))
    {
        return Section->FindValue(Name);
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
    if (FConfigSection* CurrentSection = FindSection(SectionName))
    {
        return CurrentSection;
    }

    auto Result = Sections.emplace(SectionName, FConfigSection(SectionName));
    if (Result.second)
    {
        return &Result.first->second;
    }

    return nullptr;
}

bool FConfigFile::WriteToFile()
{
    FString ConfigString;
    DumpToString(ConfigString);

    {
        FFileHandleRef File = FPlatformFile::OpenForWrite(Filename);
        if (!File)
        {
            return false;
        }

        File->Write((const uint8*)ConfigString.GetData(), ConfigString.SizeInBytes());
    }

    return false;
}

void FConfigFile::DumpToString(FString& OutString)
{
    for (auto& CurrentSection : Sections)
    {
        const FString& SectionName = CurrentSection.first;
        if (!SectionName.IsEmpty())
        {
            OutString.AppendFormat("[%s]\n", SectionName.GetCString());
        }

        FConfigSection& Section = CurrentSection.second;
        Section.DumpToString(OutString);

        OutString += '\n';
    }
}


FConfigFile* GConfig = nullptr;

FConfig* FConfig::GInstance = nullptr;

FConfig::FConfig()
    : ConfigFiles()
{ }

bool FConfig::Initialize()
{
    CHECK(GInstance == nullptr);
    
    // TODO: Only have the name of the file
    GInstance = new FConfig();
    if (FConfigFile* NewFile = GInstance->LoadFile(ENGINE_LOCATION"/Engine.ini"))
    {
        GConfig = NewFile;
    }
    else
    {
        LOG_WARNING("Did not find 'Engine.ini'");
    }

    GInstance->LoadConsoleVariables();
    return true;
}

void FConfig::Release()
{
    if (GInstance)
    {
        delete GInstance;
        GInstance = nullptr;

        // Invalidate pointer after the config is deleted
        GConfig = nullptr;
    }
}

FConfigFile* FConfig::LoadFile(const FString& Filename)
{
    TArray<CHAR> FileContents;
    {
        FFileHandleRef File = FPlatformFile::OpenForRead(Filename);
        if (!File)
        {
            return nullptr;
        }

        // Read the full file
        if (!FFileHelpers::ReadTextFile(File.Get(), FileContents))
        {
            return nullptr;
        }
    }

    // Remove all carriage returns if there are any (Easier to process)
    FileContents.RemoveAll('\r');

    // Get a config file, if the file already exists the values will be overwritten
    FConfigFile*    ConfigFile = AddConfigFile(Filename);
    FConfigSection* CurrentSection = nullptr;

    CHAR* Start = FileContents.GetData();
    while (Start && *Start)
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
                CurrentSection = ConfigFile->FindOrAddSection(SectionStart);
            }
        }
        else if (*LineStart != ';') // Check if this is a comment line
        {
            if (CHAR* EqualSign = FCString::Strchr(LineStart, '='))
            {
                *EqualSign = '\0';

                CHAR* KeyEnd = EqualSign - 1;
                while (*KeyEnd == ' ')
                    *(KeyEnd--) = '\0';

                // The parsed key
                CHAR* Key = LineStart;
                LineStart = EqualSign + 1;

                FParse::ParseWhiteSpace(&LineStart);

                // Find the end of the value
                CHAR* Value = LineStart;
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
                    CurrentSection = ConfigFile->FindOrAddSection("");
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

    return ConfigFile;
}

void FConfig::LoadConsoleVariables()
{
    FConsoleManager& ConsoleManager = FConsoleManager::Get();
    for (const auto& FilePair : ConfigFiles)
    {
        for (const auto& SectionPair : FilePair.second.Sections)
        {
            for (const auto& ValuePair : SectionPair.second.Values)
            {
                const auto& Key = ValuePair.first;
                const auto& Value = ValuePair.second;

                if (IConsoleVariable* Variable = ConsoleManager.FindConsoleVariable(Key.GetCString()))
                {
                    Variable->SetString(Value.CurrentValue.GetCString(), EConsoleVariableFlags::SetByConfigFile);
                }
            }
        }
    }
}

FConfigFile* FConfig::FindFile(const FString& Filename)
{
    auto CurrentFile = ConfigFiles.find(Filename);
    if (CurrentFile != ConfigFiles.end())
    {
        return &CurrentFile->second;
    }

    return nullptr;
}

FConfigFile* FConfig::AddConfigFile(const FString& Filename)
{
    if (FConfigFile* CurrentFile = FindFile(Filename))
    {
        return CurrentFile;
    }

    auto Result = ConfigFiles.emplace(Filename, FConfigFile());
    if (!Result.second)
    {
        return nullptr;
    }

    return &Result.first->second;
}
