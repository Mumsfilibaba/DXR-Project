#include "ConsoleManagerCommandLineTests.h"

#include <Core/Containers/String.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/Config.h>

#include "TestCommon/TestMacros.h"

static bool InitializeFromLine(const CHAR_T* Line)
{
    const CHAR_T* Args[] = { Line };
    return CommandLine::Initialize(Args, 1);
}

static EConsoleVariableFlags GetSetByFlag(const IConsoleVariable* Variable)
{
    return Variable->GetFlags() & EConsoleVariableFlags::SetByMask;
}

struct FCountingOutputDevice : public IOutputDevice
{
    virtual void Log(const FString&) override
    {
        ++NumMessages;
    }

    virtual void Log(ELogSeverity Severity, const FString&) override
    {
        ++NumMessages;
        NumErrors += (Severity == ELogSeverity::Error) ? 1 : 0;
    }

    int32 NumMessages = 0;
    int32 NumErrors   = 0;
};

bool ConsoleManagerCommandLine_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Every variable type is settable at registration"));
    {
        InitializeFromLine(TEXT("-Test.Bool=true -Test.Int=42 -Test.Float=0.25 -Test.String=Vulkan"));

        TAutoConsoleVariable<bool>    CVarBool(TEXT("Test.Bool"), TEXT(""), false);
        TAutoConsoleVariable<int32>   CVarInt(TEXT("Test.Int"), TEXT(""), 0);
        TAutoConsoleVariable<float>   CVarFloat(TEXT("Test.Float"), TEXT(""), 0.0f);
        TAutoConsoleVariable<FString> CVarString(TEXT("Test.String"), TEXT(""), TEXT("Unknown"));

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), 42);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), 0.25f);
        TEST_EXPECT(CVarString.GetValue().Equals(TEXT("Vulkan")));
    }

    TEST_SECTION(TEXT("A command-line set is tagged SetByCommandLine"));
    {
        InitializeFromLine(TEXT("-Test.String=Vulkan"));

        TAutoConsoleVariable<FString> CVarString(TEXT("Test.String"), TEXT(""), TEXT("Unknown"));
        TEST_EXPECT_EQ(GetSetByFlag(CVarString.operator->()), EConsoleVariableFlags::SetByCommandLine);

        // A variable with no matching option keeps its constructor tag
        TAutoConsoleVariable<FString> CVarUntouched(TEXT("Test.NotOnCommandLine"), TEXT(""), TEXT("Unknown"));
        TEST_EXPECT(GetSetByFlag(CVarUntouched.operator->()) != EConsoleVariableFlags::SetByCommandLine);
    }

    TEST_SECTION(TEXT("Numeric values accept the forms the console accepts"));
    {
        InitializeFromLine(TEXT("-Test.Bool=1 -Test.Int=-7 -Test.Float=-1.5"));

        TAutoConsoleVariable<bool>  CVarBool(TEXT("Test.Bool"), TEXT(""), false);
        TAutoConsoleVariable<int32> CVarInt(TEXT("Test.Int"), TEXT(""), 0);
        TAutoConsoleVariable<float> CVarFloat(TEXT("Test.Float"), TEXT(""), 0.0f);

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), -7);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), -1.5f);
    }

    TEST_SECTION(TEXT("A bare switch turns every variable type on"));
    {
        InitializeFromLine(TEXT("-Test.Bool -Test.Int -Test.Float -Test.String"));

        TAutoConsoleVariable<bool>    CVarBool(TEXT("Test.Bool"), TEXT(""), false);
        TAutoConsoleVariable<int32>   CVarInt(TEXT("Test.Int"), TEXT(""), 0);
        TAutoConsoleVariable<float>   CVarFloat(TEXT("Test.Float"), TEXT(""), 0.0f);
        TAutoConsoleVariable<FString> CVarString(TEXT("Test.String"), TEXT(""), TEXT("Unknown"));

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), 1);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), 1.0f);
        TEST_EXPECT(CVarString.GetValue().Equals(TEXT("true")));
    }

    TEST_SECTION(TEXT("Clamp ranges still apply to command-line values"));
    {
        InitializeFromLine(TEXT("-Test.Int=1000 -Test.Float=-1000.0"));

        TAutoConsoleVariable<int32> CVarInt(TEXT("Test.Int"), TEXT(""), 0, 0, 16);
        TAutoConsoleVariable<float> CVarFloat(TEXT("Test.Float"), TEXT(""), 0.0f, -1.0f, 1.0f);

        TEST_EXPECT_EQ(CVarInt.GetValue(), 16);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), -1.0f);
    }

    TEST_SECTION(TEXT("A ReadOnly variable is not changed by the command line"));
    {
        InitializeFromLine(TEXT("-Test.ReadOnly=99 -Test.ReadOnly.Switch"));

        TAutoConsoleVariable<int32> CVarReadOnly(TEXT("Test.ReadOnly"), TEXT(""), 5, EConsoleVariableFlags::ReadOnly);
        TEST_EXPECT_EQ(CVarReadOnly.GetValue(), 5);

        // The bare-switch path goes through SetAsBool, which must respect ReadOnly too
        TAutoConsoleVariable<bool> CVarReadOnlySwitch(TEXT("Test.ReadOnly.Switch"), TEXT(""), false, EConsoleVariableFlags::ReadOnly);
        TEST_EXPECT_EQ(CVarReadOnlySwitch.GetValue(), false);
    }

    TEST_SECTION(TEXT("The command line outranks the config file"));
    {
        // Stand in a config of our own so the assertion does not depend on Engine.ini
        FIniFile TestConfig;
        FIniSection& Section = TestConfig.Sections.FindOrAdd(TEXT(""));
        Section.Values.Add(TEXT("Test.Contested"), FIniValue(FString(TEXT("FromConfig"))));
        Section.Values.Add(TEXT("Test.ConfigOnly"), FIniValue(FString(TEXT("FromConfig"))));

        FConfig TestStack;
        TestStack.SetFile(EConfigFile::Engine, &TestConfig);

        FConfig* PreviousConfig = GConfig;
        GConfig = &TestStack;

        {
            InitializeFromLine(TEXT("-Test.Contested=FromCommandLine"));

            TAutoConsoleVariable<FString> CVarContested(TEXT("Test.Contested"), TEXT(""), TEXT("Default"));
            TEST_EXPECT(CVarContested.GetValue().Equals(TEXT("FromCommandLine")));
            TEST_EXPECT_EQ(GetSetByFlag(CVarContested.operator->()), EConsoleVariableFlags::SetByCommandLine);

            // Without a matching option the config file is still applied
            TAutoConsoleVariable<FString> CVarConfigOnly(TEXT("Test.ConfigOnly"), TEXT(""), TEXT("Default"));
            TEST_EXPECT(CVarConfigOnly.GetValue().Equals(TEXT("FromConfig")));
            TEST_EXPECT_EQ(GetSetByFlag(CVarConfigOnly.operator->()), EConsoleVariableFlags::SetByConfigFile);
        }

        GConfig = PreviousConfig;
    }

    TEST_SECTION(TEXT("The command line outranks every layer, not just the lowest one"));
    {
        FIniFile EngineFile;
        EngineFile.Sections.FindOrAdd(TEXT("")).Values.Add(TEXT("Test.Layered"), FIniValue(FString(TEXT("FromEngine"))));

        FIniFile EditorFile;
        EditorFile.Sections.FindOrAdd(TEXT("")).Values.Add(TEXT("Test.Layered"), FIniValue(FString(TEXT("FromEditor"))));

        FConfig TestStack;
        TestStack.SetFile(EConfigFile::Engine, &EngineFile);
        TestStack.SetFile(EConfigFile::Editor, &EditorFile);

        FConfig* PreviousConfig = GConfig;
        GConfig = &TestStack;

        {
            InitializeFromLine(TEXT("-Test.Layered=FromCommandLine"));

            TAutoConsoleVariable<FString> CVarLayered(TEXT("Test.Layered"), TEXT(""), TEXT("Default"));
            TEST_EXPECT(CVarLayered.GetValue().Equals(TEXT("FromCommandLine")));
            TEST_EXPECT_EQ(GetSetByFlag(CVarLayered.operator->()), EConsoleVariableFlags::SetByCommandLine);
        }

        GConfig = PreviousConfig;
    }

    TEST_SECTION(TEXT("A partial name match does not suppress the config file"));
    {
        FIniFile TestConfig;
        FIniSection& Section = TestConfig.Sections.FindOrAdd(TEXT(""));
        Section.Values.Add(TEXT("Test.Type"), FIniValue(FString(TEXT("FromConfig"))));

        FConfig TestStack;
        TestStack.SetFile(EConfigFile::Engine, &TestConfig);

        FConfig* PreviousConfig = GConfig;
        GConfig = &TestStack;

        {
            // 'Test.TypeExtra' must not be mistaken for 'Test.Type'
            InitializeFromLine(TEXT("-Test.TypeExtra=FromCommandLine"));

            TAutoConsoleVariable<FString> CVarType(TEXT("Test.Type"), TEXT(""), TEXT("Default"));
            TEST_EXPECT(CVarType.GetValue().Equals(TEXT("FromConfig")));
        }

        GConfig = PreviousConfig;
    }

    TEST_SECTION(TEXT("Variables registered before the command line is parsed still pick it up"));
    {
        InitializeFromLine(TEXT(""));

        TAutoConsoleVariable<bool>    CVarBool(TEXT("Test.Deferred.Bool"), TEXT(""), false);
        TAutoConsoleVariable<int32>   CVarInt(TEXT("Test.Deferred.Int"), TEXT(""), 0);
        TAutoConsoleVariable<float>   CVarFloat(TEXT("Test.Deferred.Float"), TEXT(""), 0.0f);
        TAutoConsoleVariable<FString> CVarString(TEXT("Test.Deferred.String"), TEXT(""), TEXT("Unknown"));

        // Nothing was on the command line at registration time
        TEST_EXPECT_EQ(CVarString.GetValue(), FString(TEXT("Unknown")));

        InitializeFromLine(
            TEXT("-Test.Deferred.Bool=true -Test.Deferred.Int=42 "
            "-Test.Deferred.Float=0.25 -Test.Deferred.String=Vulkan"));

        FConsoleManager::Get().LoadFromCommandLine();

        TEST_EXPECT_EQ(CVarBool.GetValue(), true);
        TEST_EXPECT_EQ(CVarInt.GetValue(), 42);
        TEST_EXPECT_EQ(CVarFloat.GetValue(), 0.25f);
        TEST_EXPECT(CVarString.GetValue().Equals(TEXT("Vulkan")));

        TEST_EXPECT_EQ(GetSetByFlag(CVarString.operator->()), EConsoleVariableFlags::SetByCommandLine);
    }

    TEST_SECTION(TEXT("The deferred pass honours bare switches and clamp ranges"));
    {
        InitializeFromLine(TEXT(""));

        TAutoConsoleVariable<bool>  CVarSwitch(TEXT("Test.Deferred.Switch"), TEXT(""), false);
        TAutoConsoleVariable<int32> CVarClamped(TEXT("Test.Deferred.Clamped"), TEXT(""), 0, 0, 16);

        InitializeFromLine(TEXT("-Test.Deferred.Switch -Test.Deferred.Clamped=1000"));
        FConsoleManager::Get().LoadFromCommandLine();

        TEST_EXPECT_EQ(CVarSwitch.GetValue(), true);
        TEST_EXPECT_EQ(CVarClamped.GetValue(), 16);
    }

    TEST_SECTION(TEXT("The deferred pass leaves unmatched variables alone"));
    {
        InitializeFromLine(TEXT(""));

        TAutoConsoleVariable<FString> CVarUntouched(TEXT("Test.Deferred.Untouched"), TEXT(""), TEXT("Default"));

        InitializeFromLine(TEXT("-Test.Deferred.SomethingElse=Value"));
        FConsoleManager::Get().LoadFromCommandLine();

        TEST_EXPECT(CVarUntouched.GetValue().Equals(TEXT("Default")));
        TEST_EXPECT(GetSetByFlag(CVarUntouched.operator->()) != EConsoleVariableFlags::SetByCommandLine);
    }

    TEST_SECTION(TEXT("ExecuteCommand reports whether the line did anything"));
    {
        InitializeFromLine(TEXT(""));

        int32 NumCalls = 0;
        FAutoConsoleCommand CCmdCounter(TEXT("Test.Execute.Counter"), TEXT(""), FConsoleCommandDelegate::CreateLambda([&NumCalls](FStringView)
        {
            ++NumCalls;
        }));

        TAutoConsoleVariable<int32> CVarInt(TEXT("Test.Execute.Int"), TEXT(""), 0);

        FCountingOutputDevice OutputDevice;
        TEST_EXPECT(FConsoleManager::Get().ExecuteCommand(OutputDevice, TEXT("Test.Execute.Counter")));
        TEST_EXPECT(FConsoleManager::Get().ExecuteCommand(OutputDevice, TEXT("Test.Execute.Counter with arguments")));
        TEST_EXPECT_EQ(NumCalls, 2);

        TEST_EXPECT(FConsoleManager::Get().ExecuteCommand(OutputDevice, TEXT("Test.Execute.Int 7")));
        TEST_EXPECT_EQ(CVarInt.GetValue(), 7);
        TEST_EXPECT_EQ(OutputDevice.NumErrors, 0);

        TEST_EXPECT(!FConsoleManager::Get().ExecuteCommand(OutputDevice, TEXT("Test.Execute.Missing")));
        TEST_EXPECT(!FConsoleManager::Get().ExecuteCommand(OutputDevice, TEXT("Test.Execute.Missing 1")));
        TEST_EXPECT(!FConsoleManager::Get().ExecuteCommand(OutputDevice, TEXT("Test.Execute.Int NotANumber")));
        TEST_EXPECT_EQ(CVarInt.GetValue(), 7);
        TEST_EXPECT_EQ(OutputDevice.NumErrors, 3);
    }

    // Leave the command line empty so later suites start from a clean slate
    InitializeFromLine(TEXT(""));

    TEST_END();
}
