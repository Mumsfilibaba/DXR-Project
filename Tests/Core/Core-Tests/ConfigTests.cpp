#include "ConfigTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Misc/Config.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

// Every layer is built in memory, so none of this depends on the files at the repository root
static FIniFile ParseIni(const CHAR* InText)
{
    // The parser walks the buffer in place, so the null-terminator has to be copied along
    TArray<CHAR> Text(InText, CString::Strlen(InText) + 1);

    FIniFile IniFile;
    IniFile.ParseFromText(Text);
    return IniFile;
}

bool Config_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Every layer overrides the ones below it");
    {
        FIniFile EngineFile   = ParseIni("[Test]\nContested = FromEngine\n");
        FIniFile GameFile     = ParseIni("[Test]\nContested = FromGame\n");
        FIniFile EditorFile   = ParseIni("[Test]\nContested = FromEditor\n");
        FIniFile PlatformFile = ParseIni("[Test]\nContested = FromPlatform\n");

        FConfig ConfigStack;
        String  Value;

        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        TEST_EXPECT(ConfigStack.GetString("Test", "Contested", Value));
        TEST_EXPECT(Value == "FromEngine");

        ConfigStack.SetFile(EConfigFile::Game, &GameFile);
        TEST_EXPECT(ConfigStack.GetString("Test", "Contested", Value));
        TEST_EXPECT(Value == "FromGame");

        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);
        TEST_EXPECT(ConfigStack.GetString("Test", "Contested", Value));
        TEST_EXPECT(Value == "FromEditor");

        ConfigStack.SetFile(EConfigFile::Platform, &PlatformFile);
        TEST_EXPECT(ConfigStack.GetString("Test", "Contested", Value));
        TEST_EXPECT(Value == "FromPlatform");
    }

    TEST_SECTION("A key that only the lowest layer has stays visible");
    {
        FIniFile EngineFile = ParseIni("[Test]\nEngineOnly = 1280\nContested = FromEngine\n");
        FIniFile EditorFile = ParseIni("[Test]\nContested = FromEditor\n");

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);

        String Value;
        TEST_EXPECT(ConfigStack.GetString("Test", "EngineOnly", Value));
        TEST_EXPECT(Value == "1280");

        TEST_EXPECT(!ConfigStack.GetString("Test", "NotPresent", Value));
        TEST_EXPECT(ConfigStack.FindValue("Test", "NotPresent") == nullptr);
    }

    TEST_SECTION("An absent layer leaves the layer below it in place");
    {
        FIniFile EngineFile   = ParseIni("[Test]\nEngineOnly = 1280\nContested = FromEngine\n");
        FIniFile PlatformFile = ParseIni("[Test]\nContested = FromPlatform\n");

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Platform, &PlatformFile);

        TEST_EXPECT(ConfigStack.GetFile(EConfigFile::Engine) == &EngineFile);
        TEST_EXPECT(ConfigStack.GetFile(EConfigFile::Game) == nullptr);
        TEST_EXPECT(ConfigStack.GetFile(EConfigFile::Editor) == nullptr);

        String Value;
        TEST_EXPECT(ConfigStack.GetString("Test", "Contested", Value));
        TEST_EXPECT(Value == "FromPlatform");

        TEST_EXPECT(ConfigStack.GetString("Test", "EngineOnly", Value));
        TEST_EXPECT(Value == "1280");
    }

    TEST_SECTION("The typed getters resolve through the layers");
    {
        FIniFile EngineFile = ParseIni("[Test]\nWidth = 1280\nScale = 0.5\nEnabled = false\n");
        FIniFile EditorFile = ParseIni("[Test]\nWidth = 2560\nEnabled = true\n");

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);

        int32 Width = 0;
        TEST_EXPECT(ConfigStack.GetInt("Test", "Width", Width));
        TEST_EXPECT_EQ(Width, 2560);

        bool bEnabled = false;
        TEST_EXPECT(ConfigStack.GetBool("Test", "Enabled", bEnabled));
        TEST_EXPECT_EQ(bEnabled, true);

        float Scale = 0.0f;
        TEST_EXPECT(ConfigStack.GetFloat("Test", "Scale", Scale));
        TEST_EXPECT_EQ(Scale, 0.5f);
    }

    TEST_SECTION("An empty section name searches every section of every layer");
    {
        FIniFile EngineFile   = ParseIni("[Engine]\nTest.Config.Sectioned = FromEngine\n");
        FIniFile PlatformFile = ParseIni("[RHI]\nTest.Config.Sectioned = FromPlatform\n");

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Platform, &PlatformFile);

        String Value;
        TEST_EXPECT(ConfigStack.GetString("", "Test.Config.Sectioned", Value));
        TEST_EXPECT(Value == "FromPlatform");
    }

    TEST_SECTION("Loading the console variables leaves the highest layer's value");
    {
        FIniFile EngineFile = ParseIni("[Test]\nTest.Config.CVar = FromEngine\n");
        FIniFile GameFile   = ParseIni("[Test]\nTest.Config.CVar = FromGame\n");
        FIniFile EditorFile = ParseIni("[Test]\nTest.Config.CVar = FromEditor\n");

        FConfig ConfigStack;
        ConfigStack.SetFile(EConfigFile::Engine, &EngineFile);
        ConfigStack.SetFile(EConfigFile::Game, &GameFile);
        ConfigStack.SetFile(EConfigFile::Editor, &EditorFile);

        TAutoConsoleVariable<String> CVar("Test.Config.CVar", "", "Default");
        TEST_EXPECT(CVar.GetValue().Equals("Default"));

        ConfigStack.LoadConsoleVariables();

        TEST_EXPECT(CVar.GetValue().Equals("FromEditor"));
        TEST_EXPECT_EQ(CVar->GetFlags() & EConsoleVariableFlags::SetByMask, EConsoleVariableFlags::SetByConfigFile);
    }

    TEST_END();
}
