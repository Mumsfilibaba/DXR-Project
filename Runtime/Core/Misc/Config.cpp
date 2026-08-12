#include "Core/Misc/Config.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/BuildInfo.h"
#include "Core/Misc/Paths.h"

FConfig* GConfig = nullptr;

FConfig::FConfig()
    : ConfigFiles()
{
    Memory::Memzero(LayerFiles, sizeof(LayerFiles));
}

FConfig::~FConfig() = default;

bool FConfig::Initialize()
{
    CHECK(GConfig == nullptr);

    GConfig = new FConfig();

    const String ConfigDir = Paths::GetEngineDir();
    GConfig->AddLayer(EConfigFile::Engine, String::Printf("%s/Engine.ini", *ConfigDir));
    GConfig->AddLayer(EConfigFile::Game, String::Printf("%s/Game.ini", *ConfigDir));

#if EDITOR_BUILD
    GConfig->AddLayer(EConfigFile::Editor, String::Printf("%s/Editor.ini", *ConfigDir));
#endif

    GConfig->AddLayer(EConfigFile::Platform, String::Printf("%s/%s.ini", *ConfigDir, BuildInfo::GetPlatformName()));

    GConfig->LoadConsoleVariables();
    return true;
}

void FConfig::Release()
{
    if (GConfig)
    {
        delete GConfig;
        GConfig = nullptr;
    }
}

FIniFile* FConfig::LoadFile(const String& Filename)
{
    // Get a config file, if the file already exists the values will be overwritten
    FIniFile& ConfigFile = ConfigFiles.FindOrAdd(Filename);
    if (!ConfigFile.LoadFromFile(Filename))
    {
        ConfigFiles.Remove(Filename);
        return nullptr;
    }

    return &ConfigFile;
}

FIniFile* FConfig::GetFile(EConfigFile::Type ConfigFile)
{
    CHECK(ConfigFile < EConfigFile::Count);
    return LayerFiles[ConfigFile];
}

void FConfig::SetFile(EConfigFile::Type ConfigFile, FIniFile* File)
{
    CHECK(ConfigFile < EConfigFile::Count);
    LayerFiles[ConfigFile] = File;
}

FIniValue* FConfig::FindValue(const CHAR* SectionName, const CHAR* Name)
{
    for (int32 Index = EConfigFile::Count - 1; Index >= 0; --Index)
    {
        if (FIniFile* File = LayerFiles[Index])
        {
            if (FIniValue* Value = File->FindValue(SectionName, Name))
            {
                return Value;
            }
        }
    }

    return nullptr;
}

bool FConfig::GetString(const CHAR* SectionName, const CHAR* Name, String& OutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        OutValue = Value->CurrentValue;
        return true;
    }
    else
    {
        return false;
    }
}

bool FConfig::GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        return TTypeFromString<int32>::FromString(Value->CurrentValue, OutValue);
    }
    else
    {
        return false;
    }
}

bool FConfig::GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        return TTypeFromString<float>::FromString(Value->CurrentValue, OutValue);
    }
    else
    {
        return false;
    }
}

bool FConfig::GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        return TTypeFromString<bool>::FromString(Value->CurrentValue, bOutValue);
    }
    else
    {
        return false;
    }
}

void FConfig::LoadConsoleVariables()
{
    FConsoleManager& ConsoleManager = FConsoleManager::Get();

    for (int32 Index = 0; Index < EConfigFile::Count; ++Index)
    {
        FIniFile* File = LayerFiles[Index];
        if (!File)
        {
            continue;
        }

        for (auto Section : File->Sections)
        {
            for (auto Value : Section.Second.Values)
            {
                if (IConsoleVariable* Variable = ConsoleManager.FindConsoleVariable(*Value.First))
                {
                    // The command line outranks the config file, so never downgrade 
                    // a variable that a '-Name=Value' option has already claimed.
                    const EConsoleVariableFlags SetBy = Variable->GetFlags() & EConsoleVariableFlags::SetByMask;
                    if (SetBy != EConsoleVariableFlags::SetByCommandLine)
                    {
                        Variable->SetString(*Value.Second.CurrentValue, EConsoleVariableFlags::SetByConfigFile);
                    }
                }
            }
        }
    }
}

FIniFile* FConfig::AddLayer(EConfigFile::Type ConfigFile, const String& Filename)
{
    FIniFile* File = LoadFile(Filename);
    if (File)
    {
        SetFile(ConfigFile, File);
    }
    else if (ConfigFile == EConfigFile::Engine)
    {
        LOG_WARNING("Did not find '%s'", *Filename);
    }
    else
    {
        LOG_INFO("Did not find '%s'", *Filename);
    }

    return File;
}
