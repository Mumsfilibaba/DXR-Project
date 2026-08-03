#include "Core/Misc/Config.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"

FIniFile* GConfig = nullptr;

FConfig* FConfig::GlobalConfig = nullptr;

FConfig::FConfig()
    : ConfigFiles()
{
}

bool FConfig::Initialize()
{
    CHECK(GlobalConfig == nullptr);
    
    // TODO: Only have the name of the file
    GlobalConfig = new FConfig();
    if (FIniFile* NewFile = GlobalConfig->LoadFile(ENGINE_LOCATION"/Engine.ini"))
    {
        GConfig = NewFile;
    }
    else
    {
        LOG_WARNING("Did not find 'Engine.ini'");
    }

    GlobalConfig->LoadConsoleVariables();
    return true;
}

void FConfig::Release()
{
    if (GlobalConfig)
    {
        delete GlobalConfig;
        GlobalConfig = nullptr;

        // Invalidate pointer after the config is deleted
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

void FConfig::LoadConsoleVariables()
{
    FConsoleManager& ConsoleManager = FConsoleManager::Get();
    for (auto File : ConfigFiles)
    {
        for (auto Section : File.Second.Sections)
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
