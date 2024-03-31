#include "EngineConfig.h"
#include "Parse.h"
#include "OutputDeviceLogger.h"
#include "ConsoleManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"

FConfigSection::FConfigSection()
    : Name()
    , Values()
{
}

FConfigSection::FConfigSection(const CHAR* InName)
    : Name(InName)
    , Values()
{
}

void FConfigSection::Restore()
{
    for (auto Value : Values)
    {
        Value.Second.Restore();
    }
}

void FConfigSection::DumpToString(FString& OutString)
{
    for (auto ValuePair : Values)
    {
        OutString.AppendFormat("%s=%s\n", ValuePair.First.GetCString(), ValuePair.Second.CurrentValue.GetCString());
    }
}


bool FConfigFile::SetString(const CHAR* SectionName, const CHAR* Name, const FString& NewValue)
{
    if (FConfigValue* Value = FindValue(SectionName, Name))
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
    if (FConfigValue* Value = FindValue(SectionName, Name))
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
    if (FConfigValue* Value = FindValue(SectionName, Name))
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
    if (FConfigValue* Value = FindValue(SectionName, Name))
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
    for (auto CurrentSection : Sections)
    {
        if (FConfigValue* Value = CurrentSection.Second.Values.Find(Key))
        {
            return Value;
        }
    }
    
    return nullptr;
}

ENABLE_UNREACHABLE_CODE_WARNING

FConfigValue* FConfigFile::FindValue(const CHAR* SectionName, const CHAR* Name)
{
    const bool bIsGlobal = !SectionName || FCString::Strcmp(SectionName, "") == 0;
    if (bIsGlobal)
    {
        return FindValue(Name);
    }
    else if (FConfigSection* Section = Sections.Find(SectionName))
    {
        return Section->Values.Find(Name);
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

        File->Write((const uint8*)ConfigString.Data(), ConfigString.SizeInBytes());
    }

    return false;
}

void FConfigFile::DumpToString(FString& OutString)
{
    for (auto CurrentSection : Sections)
    {
        if (!CurrentSection.First.IsEmpty())
        {
            OutString.AppendFormat("[%s]\n", CurrentSection.First.GetCString());
        }
        
        CurrentSection.Second.DumpToString(OutString);
        OutString += '\n';
    }
}


FConfigFile* GConfig = nullptr;

FConfig* FConfig::GInstance = nullptr;

FConfig::FConfig()
    : ConfigFiles()
{
}

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
    FileContents.Remove('\r');

    // Get a config file, if the file already exists the values will be overwritten
    FConfigFile*    ConfigFile     = &ConfigFiles.FindOrAdd(Filename);
    FConfigSection* CurrentSection = nullptr;
    
    CHAR* Start = FileContents.Data();
    while (Start && *Start)
    {
        // Skip newline chars
        while (*Start == '\n')
        {
            ++Start;
        }

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
                
                FConfigSection& Section = ConfigFile->Sections.FindOrAdd(SectionStart, FConfigSection(SectionStart));
                CurrentSection = &Section;
            }
        }
        else if (*LineStart != ';') // Check if this is a comment line
        {
            if (CHAR* EqualSign = FCString::Strchr(LineStart, '='))
            {
                *EqualSign = '\0';

                CHAR* KeyEnd = EqualSign - 1;
                while (*KeyEnd == ' ')
                {
                    *(KeyEnd--) = '\0';
                }

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
                {
                    *(ValueEnd--) = '\0';
                }

                // If there are no section, use the global one
                if (!CurrentSection)
                {
                    FConfigSection& Section = ConfigFile->Sections.FindOrAdd("");
                    CurrentSection = &Section;
                }

                // The parsed value
                if (FConfigValue* CurrentValue = CurrentSection->Values.Find(Key))
                {
                    *CurrentValue = FConfigValue(Value);
                }
                else
                {
                    CurrentSection->Values.Add(Key, FConfigValue(Value));
                }
            }
        }
    }

    return ConfigFile;
}

void FConfig::LoadConsoleVariables()
{
    FConsoleManager& ConsoleManager = FConsoleManager::Get();
    for (auto File : ConfigFiles)
    {
        for (auto Section : File.Second.Sections)
        {
            for (auto Value : Section.Second.Values)
            {
                if (IConsoleVariable* Variable = ConsoleManager.FindConsoleVariable(Value.First.GetCString()))
                {
                    Variable->SetString(Value.Second.CurrentValue.GetCString(), EConsoleVariableFlags::SetByConfigFile);
                }
            }
        }
    }
}
