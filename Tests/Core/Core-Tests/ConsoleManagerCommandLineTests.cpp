#include "ConsoleManagerCommandLineTests.h"

#include <Core/Containers/String.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/Config.h>

#include "TestCommon/TestMacros.h"

static bool InitializeFromLine(const CHAR* Line)
{
    const CHAR* Args[] = { Line };
    return CommandLine::Initialize(Args, 1);
}

static EConsoleVariableFlags GetSetByFlag(const IConsoleVariable* Variable)
{
    return Variable->GetFlags() & EConsoleVariableFlags::SetByMask;
}

bool ConsoleManagerCommandLine_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Every variable type is settable at registration");
    {
        InitializeFromLine("-Test.Bool=true -Test.Int=42 -Test.Float=0.25 -Test.String=Vulkan");

        TAutoConsoleVariable<bool>   CVarBool("Test.Bool", "", false);
        TAutoConsoleVariable<int32>  CVarInt("Test.Int", "", 0);
        TAutoConsoleVariable<float>  CVarFloat("Test.Float", "", 0.0f);
        TAutoConsoleVariable<String> CVarString("Test.String", "", "Unknown");

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), 42);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), 0.25f);
        TEST_EXPECT(CVarString.GetValue().Equals("Vulkan"));
    }

    TEST_SECTION("A command-line set is tagged SetByCommandLine");
    {
        InitializeFromLine("-Test.String=Vulkan");

        TAutoConsoleVariable<String> CVarString("Test.String", "", "Unknown");
        TEST_EXPECT_EQ(GetSetByFlag(CVarString.operator->()), EConsoleVariableFlags::SetByCommandLine);

        // A variable with no matching option keeps its constructor tag
        TAutoConsoleVariable<String> CVarUntouched("Test.NotOnCommandLine", "", "Unknown");
        TEST_EXPECT(GetSetByFlag(CVarUntouched.operator->()) != EConsoleVariableFlags::SetByCommandLine);
    }

    TEST_SECTION("Numeric values accept the forms the console accepts");
    {
        InitializeFromLine("-Test.Bool=1 -Test.Int=-7 -Test.Float=-1.5");

        TAutoConsoleVariable<bool>  CVarBool("Test.Bool", "", false);
        TAutoConsoleVariable<int32> CVarInt("Test.Int", "", 0);
        TAutoConsoleVariable<float> CVarFloat("Test.Float", "", 0.0f);

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), -7);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), -1.5f);
    }

    TEST_SECTION("A bare switch turns every variable type on");
    {
        InitializeFromLine("-Test.Bool -Test.Int -Test.Float -Test.String");

        TAutoConsoleVariable<bool>   CVarBool("Test.Bool", "", false);
        TAutoConsoleVariable<int32>  CVarInt("Test.Int", "", 0);
        TAutoConsoleVariable<float>  CVarFloat("Test.Float", "", 0.0f);
        TAutoConsoleVariable<String> CVarString("Test.String", "", "Unknown");

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), 1);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), 1.0f);
        TEST_EXPECT(CVarString.GetValue().Equals("true"));
    }

    TEST_SECTION("Clamp ranges still apply to command-line values");
    {
        InitializeFromLine("-Test.Int=1000 -Test.Float=-1000.0");

        TAutoConsoleVariable<int32> CVarInt("Test.Int", "", 0, 0, 16);
        TAutoConsoleVariable<float> CVarFloat("Test.Float", "", 0.0f, -1.0f, 1.0f);

        TEST_EXPECT_EQ(CVarInt.GetValue(), 16);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), -1.0f);
    }

    TEST_SECTION("A ReadOnly variable is not changed by the command line");
    {
        InitializeFromLine("-Test.ReadOnly=99 -Test.ReadOnly.Switch");

        TAutoConsoleVariable<int32> CVarReadOnly("Test.ReadOnly", "", 5, EConsoleVariableFlags::ReadOnly);
        TEST_EXPECT_EQ(CVarReadOnly.GetValue(), 5);

        // The bare-switch path goes through SetAsBool, which must respect ReadOnly too
        TAutoConsoleVariable<bool> CVarReadOnlySwitch("Test.ReadOnly.Switch", "", false, EConsoleVariableFlags::ReadOnly);
        TEST_EXPECT_EQ(CVarReadOnlySwitch.GetValue(), false);
    }

    TEST_SECTION("The command line outranks the config file");
    {
        // Stand in a config of our own so the assertion does not depend on Engine.ini
        FIniFile TestConfig;
        FIniSection& Section = TestConfig.Sections.FindOrAdd("");
        Section.Values.Add("Test.Contested", FIniValue(String("FromConfig")));
        Section.Values.Add("Test.ConfigOnly", FIniValue(String("FromConfig")));

        FIniFile* PreviousConfig = GConfig;
        GConfig = &TestConfig;

        {
            InitializeFromLine("-Test.Contested=FromCommandLine");

            TAutoConsoleVariable<String> CVarContested("Test.Contested", "", "Default");
            TEST_EXPECT(CVarContested.GetValue().Equals("FromCommandLine"));
            TEST_EXPECT_EQ(GetSetByFlag(CVarContested.operator->()), EConsoleVariableFlags::SetByCommandLine);

            // Without a matching option the config file is still applied
            TAutoConsoleVariable<String> CVarConfigOnly("Test.ConfigOnly", "", "Default");
            TEST_EXPECT(CVarConfigOnly.GetValue().Equals("FromConfig"));
            TEST_EXPECT_EQ(GetSetByFlag(CVarConfigOnly.operator->()), EConsoleVariableFlags::SetByConfigFile);
        }

        GConfig = PreviousConfig;
    }

    TEST_SECTION("A partial name match does not suppress the config file");
    {
        FIniFile TestConfig;
        FIniSection& Section = TestConfig.Sections.FindOrAdd("");
        Section.Values.Add("Test.Type", FIniValue(String("FromConfig")));

        FIniFile* PreviousConfig = GConfig;
        GConfig = &TestConfig;

        {
            // 'Test.TypeExtra' must not be mistaken for 'Test.Type'
            InitializeFromLine("-Test.TypeExtra=FromCommandLine");

            TAutoConsoleVariable<String> CVarType("Test.Type", "", "Default");
            TEST_EXPECT(CVarType.GetValue().Equals("FromConfig"));
        }

        GConfig = PreviousConfig;
    }

    TEST_SECTION("Variables registered before the command line is parsed still pick it up");
    {
        InitializeFromLine("");

        TAutoConsoleVariable<bool>   CVarBool("Test.Deferred.Bool", "", false);
        TAutoConsoleVariable<int32>  CVarInt("Test.Deferred.Int", "", 0);
        TAutoConsoleVariable<float>  CVarFloat("Test.Deferred.Float", "", 0.0f);
        TAutoConsoleVariable<String> CVarString("Test.Deferred.String", "", "Unknown");

        // Nothing was on the command line at registration time
        TEST_EXPECT_EQ(CVarString.GetValue(), String("Unknown"));

        InitializeFromLine(
            "-Test.Deferred.Bool=true -Test.Deferred.Int=42 "
            "-Test.Deferred.Float=0.25 -Test.Deferred.String=Vulkan");

        FConsoleManager::Get().LoadConsoleVariablesFromCommandLine();

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), 42);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), 0.25f);
        TEST_EXPECT(CVarString.GetValue().Equals("Vulkan"));

        TEST_EXPECT_EQ(GetSetByFlag(CVarString.operator->()), EConsoleVariableFlags::SetByCommandLine);
    }

    TEST_SECTION("The deferred pass honours bare switches and clamp ranges");
    {
        InitializeFromLine("");

        TAutoConsoleVariable<bool>  CVarSwitch("Test.Deferred.Switch", "", false);
        TAutoConsoleVariable<int32> CVarClamped("Test.Deferred.Clamped", "", 0, 0, 16);

        InitializeFromLine("-Test.Deferred.Switch -Test.Deferred.Clamped=1000");
        FConsoleManager::Get().LoadConsoleVariablesFromCommandLine();

        TEST_EXPECT_EQ(CVarSwitch.GetValue(), true);
        TEST_EXPECT_EQ(CVarClamped.GetValue(), 16);
    }

    TEST_SECTION("The deferred pass leaves unmatched variables alone");
    {
        InitializeFromLine("");

        TAutoConsoleVariable<String> CVarUntouched("Test.Deferred.Untouched", "", "Default");

        InitializeFromLine("-Test.Deferred.SomethingElse=Value");
        FConsoleManager::Get().LoadConsoleVariablesFromCommandLine();

        TEST_EXPECT(CVarUntouched.GetValue().Equals("Default"));
        TEST_EXPECT(GetSetByFlag(CVarUntouched.operator->()) != EConsoleVariableFlags::SetByCommandLine);
    }

    // Leave the command line empty so later suites start from a clean slate
    InitializeFromLine("");

    TEST_END();
}
