#include "ConfigTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Misc/Config.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

// Every layer is built in memory, so none of this depends on the files at the repository root
static FIniFile ParseIni(const CHAR_T* InText)
{
    // The parser walks the buffer in place, so the null-terminator has to be copied along
    TArray<CHAR_T> Text(InText, CString::Strlen(InText) + 1);

    FIniFile IniFile;
    IniFile.ParseFromText(Text);
    return IniFile;
}

bool Config_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Every layer overrides the ones below it"));
    {
        FIniFile EngineFile   = ParseIni(TEXT("[Test]\nContested = FromEngine\n"));
        FIniFile GameFile     = ParseIni(TEXT("[Test]\nContested = FromGame\n"));
        FIniFile EditorFile   = ParseIni(TEXT("[Test]\nContested = FromEditor\n"));
        FIniFile PlatformFile = ParseIni(TEXT("[Test]\nContested = FromPlatform\n"));

        FConfig ConfigStack;
        FString Value;

        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("Contested"), Value));
        TEST_EXPECT(Value == TEXT("FromEngine"));

        ConfigStack.SetFile(EConfigFile::Game, &GameFile);
        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("Contested"), Value));
        TEST_EXPECT(Value == TEXT("FromGame"));

        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);
        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("Contested"), Value));
        TEST_EXPECT(Value == TEXT("FromEditor"));

        ConfigStack.SetFile(EConfigFile::Platform, &PlatformFile);
        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("Contested"), Value));
        TEST_EXPECT(Value == TEXT("FromPlatform"));
    }

    TEST_SECTION(TEXT("A key that only the lowest layer has stays visible"));
    {
        FIniFile EngineFile = ParseIni(TEXT("[Test]\nEngineOnly = 1280\nContested = FromEngine\n"));
        FIniFile EditorFile = ParseIni(TEXT("[Test]\nContested = FromEditor\n"));

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);

        FString Value;
        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("EngineOnly"), Value));
        TEST_EXPECT(Value == TEXT("1280"));

        TEST_EXPECT(!ConfigStack.GetString(TEXT("Test"), TEXT("NotPresent"), Value));
        TEST_EXPECT(ConfigStack.FindValue(TEXT("Test"), TEXT("NotPresent")) == nullptr);
    }

    TEST_SECTION(TEXT("An absent layer leaves the layer below it in place"));
    {
        FIniFile EngineFile   = ParseIni(TEXT("[Test]\nEngineOnly = 1280\nContested = FromEngine\n"));
        FIniFile PlatformFile = ParseIni(TEXT("[Test]\nContested = FromPlatform\n"));

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Platform, &PlatformFile);

        TEST_EXPECT(ConfigStack.GetFile(EConfigFile::Engine) == &EngineFile);
        TEST_EXPECT(ConfigStack.GetFile(EConfigFile::Game) == nullptr);
        TEST_EXPECT(ConfigStack.GetFile(EConfigFile::Editor) == nullptr);

        FString Value;
        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("Contested"), Value));
        TEST_EXPECT(Value == TEXT("FromPlatform"));

        TEST_EXPECT(ConfigStack.GetString(TEXT("Test"), TEXT("EngineOnly"), Value));
        TEST_EXPECT(Value == TEXT("1280"));
    }

    TEST_SECTION(TEXT("The typed getters resolve through the layers"));
    {
        FIniFile EngineFile = ParseIni(TEXT("[Test]\nWidth = 1280\nScale = 0.5\nEnabled = false\n"));
        FIniFile EditorFile = ParseIni(TEXT("[Test]\nWidth = 2560\nEnabled = true\n"));

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);

        int32 Width = 0;
        TEST_EXPECT(ConfigStack.GetInt(TEXT("Test"), TEXT("Width"), Width));
        TEST_EXPECT_EQ(Width, 2560);

        bool bEnabled = false;
        TEST_EXPECT(ConfigStack.GetBool(TEXT("Test"), TEXT("Enabled"), bEnabled));
        TEST_EXPECT_EQ(bEnabled, true);

        float Scale = 0.0f;
        TEST_EXPECT(ConfigStack.GetFloat(TEXT("Test"), TEXT("Scale"), Scale));
        TEST_EXPECT_EQ(Scale, 0.5f);
    }

    TEST_SECTION(TEXT("An empty section name searches every section of every layer"));
    {
        FIniFile EngineFile   = ParseIni(TEXT("[Engine]\nTest.Config.Sectioned = FromEngine\n"));
        FIniFile PlatformFile = ParseIni(TEXT("[RHI]\nTest.Config.Sectioned = FromPlatform\n"));

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Platform, &PlatformFile);

        FString Value;
        TEST_EXPECT(ConfigStack.GetString(TEXT(""), TEXT("Test.Config.Sectioned"), Value));
        TEST_EXPECT(Value == TEXT("FromPlatform"));
    }

    TEST_SECTION(TEXT("Loading the console variables leaves the highest layer's value"));
    {
        FIniFile EngineFile = ParseIni(TEXT("[Test]\nTest.Config.CVar = FromEngine\n"));
        FIniFile GameFile   = ParseIni(TEXT("[Test]\nTest.Config.CVar = FromGame\n"));
        FIniFile EditorFile = ParseIni(TEXT("[Test]\nTest.Config.CVar = FromEditor\n"));

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Game, &GameFile);
        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);

        TAutoConsoleVariable<FString> CVar(TEXT("Test.Config.CVar"), TEXT(""), TEXT("Default"));
        TEST_EXPECT(CVar.GetValue().Equals(TEXT("Default")));

        ConfigStack.LoadConsoleVariables();

        TEST_EXPECT(CVar.GetValue().Equals(TEXT("FromEditor")));
        TEST_EXPECT_EQ(CVar->GetFlags() & EConsoleVariableFlags::SetByMask, EConsoleVariableFlags::SetByConfigFile);
    }

    TEST_END();
}
