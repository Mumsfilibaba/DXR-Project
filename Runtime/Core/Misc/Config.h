#pragma once
#include "Core/Misc/IniFile.h"

extern CORE_API FIniFile* GConfig;

class CORE_API FConfig
{
public:
    static bool Initialize();
    static void Release();

    FIniFile* LoadFile(const String& Filename);

    void LoadConsoleVariables();

private:
    FConfig();

    TMap<String, FIniFile> ConfigFiles;
    static FConfig* GlobalConfig;
};
