#include "CommandLineTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Containers/StringView.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Templates/CString.h>

#if PLATFORM_WINDOWS
    #include <Core/Windows/WindowsPlatformMisc.h>
#endif

#include "TestCommon/TestMacros.h"

static bool InitializeFromLine(const CHAR_T* Line)
{
    TArray<FString> Arguments;
    for (const CHAR_T* Start = Line; *Start != '\0';)
    {
        const CHAR_T* End = Start;
        while (*End != '\0' && *End != ' ')
        {
            ++End;
        }

        if (End != Start)
        {
            Arguments.Emplace(Start, static_cast<int32>(End - Start));
        }

        Start = (*End == ' ') ? End + 1 : End;
    }

    TArray<const CHAR_T*> ArgumentPointers;
    for (const FString& Argument : Arguments)
    {
        ArgumentPointers.Add(*Argument);
    }

    return CommandLine::Initialize(ArgumentPointers.Data(), ArgumentPointers.Size());
}

// Real CVar names taken from across the engine, covering every name shape currently in use.
static const CHAR_T* GNameCorpus[] =
{
    TEXT("RHI.Type"),
    TEXT("RHI.EnableDebugLayer"),
    TEXT("RHI.EnableValidation"),
    TEXT("RHI.ShaderCompiler.VerboseLogging"),
    TEXT("TaskGraph.EnableRHIThread"),
    TEXT("VulkanRHI.MaxDefragMovesPerFrame"),
    TEXT("D3D12RHI.ResidencyManager.BudgetPercentage"),
    TEXT("Renderer.SSAO.Radius"),
    TEXT("Console.DumpCVars"),
    TEXT("Echo"),
    TEXT("Some_Underscored_Name"),
    TEXT("Mixed.Case_With123.Digits456"),
};

bool CommandLine_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Dotted names survive tokenization"));
    {
        InitializeFromLine(TEXT("-RHI.Type=Vulkan"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.Type"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));
    }

    TEST_SECTION(TEXT("Multi-dotted and underscored names"));
    {
        InitializeFromLine(TEXT("-RHI.ShaderCompiler.VerboseLogging=true -Some_Underscored_Name=7"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.ShaderCompiler.VerboseLogging"), Value));
        TEST_EXPECT(Value.Equals(TEXT("true")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("Some_Underscored_Name"), Value));
        TEST_EXPECT(Value.Equals(TEXT("7")));
    }

    TEST_SECTION(TEXT("Value keeps its first character"));
    {
        InitializeFromLine(TEXT("-A.B=Vulkan -C.D=1"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.B"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("C.D"), Value));
        TEST_EXPECT(Value.Equals(TEXT("1")));
    }

    TEST_SECTION(TEXT("Floats, negatives and paths are not truncated"));
    {
        InitializeFromLine(TEXT("-A.B=1.5"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.B"), Value));
        TEST_EXPECT(Value.Equals(TEXT("1.5")));

        InitializeFromLine(TEXT("-A.B=-0.5"));
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.B"), Value));
        TEST_EXPECT(Value.Equals(TEXT("-0.5")));

        InitializeFromLine(TEXT("-Log.File=C:/foo/bar.txt"));
        TEST_EXPECT(CommandLine::FindOption(TEXT("Log.File"), Value));
        TEST_EXPECT(Value.Equals(TEXT("C:/foo/bar.txt")));
    }

    TEST_SECTION(TEXT("A value runs to the end of its argument, spaces included"));
    {
        const CHAR_T* Args[] = { TEXT("-A.B=Hello World"), TEXT("-engine=C:/My Engine/"), TEXT("-C.D=2") };
        TEST_EXPECT(CommandLine::Initialize(Args, 3));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.B"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Hello World")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("engine"), Value));
        TEST_EXPECT(Value.Equals(TEXT("C:/My Engine/")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("C.D"), Value));
        TEST_EXPECT(Value.Equals(TEXT("2")));
    }

    TEST_SECTION(TEXT("An argument is one option, even when it looks like several"));
    {
        const CHAR_T* Args[] = { TEXT("-A.B=1 -C.D=2") };
        TEST_EXPECT(CommandLine::Initialize(Args, 1));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.B"), Value));
        TEST_EXPECT(Value.Equals(TEXT("1 -C.D=2")));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("C.D")));
    }

    TEST_SECTION(TEXT("Quotes inside a value are kept, since only the operating system's splitting removes quotes"));
    {
        const CHAR_T* Args[] = { TEXT("-ExecuteCommands=Echo \"Hello World\"") };
        TEST_EXPECT(CommandLine::Initialize(Args, 1));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("ExecuteCommands"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Echo \"Hello World\"")));
    }

    TEST_SECTION(TEXT("An argument that is not an option is skipped, wherever a dash sits in it"));
    {
        const CHAR_T* Args[] = { TEXT("Positional"), TEXT("C:/some-file.txt"), TEXT("-"), TEXT("--A.B=1"), TEXT("-Bad:Name=1"), TEXT("-C.D=2") };
        TEST_EXPECT(CommandLine::Initialize(Args, 6));

        FStringView Value;
        TEST_EXPECT(!CommandLine::FindOption(TEXT("file.txt")));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("A.B")));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("Bad")));
        TEST_EXPECT(CommandLine::FindOption(TEXT("C.D"), Value));
        TEST_EXPECT(Value.Equals(TEXT("2")));

        TEST_EXPECT(FStringView(CommandLine::GetOriginal()).Equals(TEXT("Positional C:/some-file.txt - --A.B=1 -Bad:Name=1 -C.D=2")));
    }

    TEST_SECTION(TEXT("Several options on one line"));
    {
        InitializeFromLine(TEXT("-RHI.Type=Vulkan -RHI.EnableDebugLayer=true -Foo.Bar=3"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.Type"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.EnableDebugLayer"), Value));
        TEST_EXPECT(Value.Equals(TEXT("true")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("Foo.Bar"), Value));
        TEST_EXPECT(Value.Equals(TEXT("3")));
    }

    TEST_SECTION(TEXT("A dash inside a value does not start a new option"));
    {
        InitializeFromLine(TEXT("-A.B=x-y -C.D=2"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.B"), Value));
        TEST_EXPECT(Value.Equals(TEXT("x-y")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("C.D"), Value));
        TEST_EXPECT(Value.Equals(TEXT("2")));

        TEST_EXPECT(!CommandLine::FindOption(TEXT("y")));
    }

    TEST_SECTION(TEXT("Matching is by whole token, not substring"));
    {
        InitializeFromLine(TEXT("-RHI.TypeExtra=1"));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("RHI.Type")));

        // A partial hit must not stop the scan from reaching the real option
        InitializeFromLine(TEXT("-RHI.TypeExtra=1 -RHI.Type=Vulkan"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.Type"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));

        // A name that only appears as a suffix of another option is not a match
        InitializeFromLine(TEXT("-Prefixed.RHI.Type=1"));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("RHI.Type")));
    }

    TEST_SECTION(TEXT("Matching is case-insensitive"));
    {
        InitializeFromLine(TEXT("-RHI.Type=Vulkan"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("rhi.type"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));
    }

    TEST_SECTION(TEXT("A bare switch is found with an empty value"));
    {
        InitializeFromLine(TEXT("-RHI.EnableDebugLayer -RHI.Type=Vulkan"));

        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.EnableDebugLayer")));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.EnableDebugLayer"), Value));
        TEST_EXPECT(Value.IsEmpty());

        // The switch must not swallow the option that follows it
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.Type"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));
    }

    TEST_SECTION(TEXT("A bare switch at the very end of the line"));
    {
        InitializeFromLine(TEXT("-RHI.EnableValidation"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.EnableValidation"), Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION(TEXT("A missing option leaves the output untouched"));
    {
        InitializeFromLine(TEXT("-RHI.Type=Vulkan"));

        FStringView Value(TEXT("sentinel"));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("Does.Not.Exist"), Value));
        TEST_EXPECT(Value.Equals(TEXT("sentinel")));
    }

    TEST_SECTION(TEXT("Argv-style arguments"));
    {
        const CHAR_T* Args[] = { TEXT("-RHI.Type=Vulkan"), TEXT("-RHI.EnableDebugLayer=true") };
        TEST_EXPECT(CommandLine::Initialize(Args, 2));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.Type"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Vulkan")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("RHI.EnableDebugLayer"), Value));
        TEST_EXPECT(Value.Equals(TEXT("true")));

        // Arguments must be separated in the raw copy, not run together
        TEST_EXPECT(FStringView(CommandLine::GetOriginal()).Equals(TEXT("-RHI.Type=Vulkan -RHI.EnableDebugLayer=true")));
    }

    TEST_SECTION(TEXT("Re-initializing replaces the previous command line"));
    {
        InitializeFromLine(TEXT("-A.Long.Option.Name=SomethingLong -B.Another=2"));
        InitializeFromLine(TEXT("-A.Long.Option.Name=Short"));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("A.Long.Option.Name"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Short")));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("B.Another")));
    }

    TEST_SECTION(TEXT("A null argument is rejected"));
    {
        const CHAR_T* Args[] = { TEXT("-A.B=1"), nullptr };
        TEST_EXPECT(!CommandLine::Initialize(Args, 2));
        TEST_EXPECT(!CommandLine::Initialize(nullptr, 1));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("A.B")));
    }

    TEST_SECTION(TEXT("Every CVar name shape round-trips"));
    {
        for (const CHAR_T* Name : GNameCorpus)
        {
            const FString Line = FString::Printf(TEXT("-%s=ProbeValue"), Name);
            InitializeFromLine(*Line);

            FStringView Value;
            const bool bRoundTripped = CommandLine::FindOption(Name, Value) && Value.Equals(TEXT("ProbeValue"));
            if (!bRoundTripped)
            {
                LOG_ERROR("CVar name '%s' did not round-trip through the CommandLine", Name);
            }

            TEST_EXPECT(bRoundTripped);
        }
    }

    TEST_SECTION(TEXT("Generated names over the legal character set"));
    {
        const CHAR_T* Alphabet = TEXT("abzABZ019._");
        for (const CHAR_T* Char = Alphabet; *Char != '\0'; ++Char)
        {
            const FString Name = FString::Printf(TEXT("Prefix%cSuffix"), *Char);
            const FString Line = FString::Printf(TEXT("-%s=ProbeValue"), *Name);
            InitializeFromLine(*Line);

            FStringView Value;
            const bool bRoundTripped = CommandLine::FindOption(*Name, Value) && Value.Equals(TEXT("ProbeValue"));
            if (!bRoundTripped)
            {
                LOG_ERROR("Generated name '%s' did not round-trip through the CommandLine", *Name);
            }

            TEST_EXPECT(bRoundTripped);
        }
    }

    TEST_SECTION(TEXT("An over-long command line truncates safely"));
    {
        FString LongLine;
        for (int32 Index = 0; Index < 200; ++Index)
        {
            LongLine.AppendPrintf(TEXT("-Long.Option%d=Value%d "), Index, Index);
        }

        TEST_EXPECT(InitializeFromLine(*LongLine));

        const uint64 OriginalLength = static_cast<uint64>(CString::Strlen(CommandLine::GetOriginal()));
        TEST_EXPECT(OriginalLength < CommandLine::MaxCommandLineLength);

        // Whatever did fit has to still be readable, and an option that did not is dropped whole rather than cut short
        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("Long.Option0"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Value0")));
        TEST_EXPECT(!CommandLine::FindOption(TEXT("Long.Option199")));

        int32 NumFound = 0;
        for (int32 Index = 0; Index < 200; ++Index)
        {
            const FString Name = FString::Printf(TEXT("Long.Option%d"), Index);
            if (CommandLine::FindOption(*Name, Value))
            {
                TEST_EXPECT(Value.Equals(*FString::Printf(TEXT("Value%d"), Index)));
                ++NumFound;
            }
        }

        TEST_EXPECT(NumFound > 0 && NumFound < 200);
    }

#if PLATFORM_WINDOWS
    TEST_SECTION(TEXT("Windows' own splitting takes a value quoted on its own and a whole quoted argument the same way"));
    {
        TArray<FString> Arguments;
        FWindowsPlatformMisc::SplitCommandLine(
            L"\"C:/Program Files/Editor.exe\" -engine=\"C:/My Engine\" \"-project=C:/My Projects/Game.dxrproject\" "
            L"-Test.String=\"Hello World\" \"-Test.Other=Hello World\" -Test.Switch -Test.Plain=1", Arguments);

        TEST_EXPECT_EQ(Arguments.Size(), 6);

        TArray<const CHAR_T*> ArgumentPointers;
        for (const FString& Argument : Arguments)
        {
            ArgumentPointers.Add(*Argument);
        }

        TEST_EXPECT(CommandLine::Initialize(ArgumentPointers.Data(), ArgumentPointers.Size()));

        FStringView Value;
        TEST_EXPECT(CommandLine::FindOption(TEXT("engine"), Value));
        TEST_EXPECT(Value.Equals(TEXT("C:/My Engine")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("project"), Value));
        TEST_EXPECT(Value.Equals(TEXT("C:/My Projects/Game.dxrproject")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("Test.String"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Hello World")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("Test.Other"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Hello World")));

        TEST_EXPECT(CommandLine::FindOption(TEXT("Test.Switch"), Value));
        TEST_EXPECT(Value.IsEmpty());

        TEST_EXPECT(CommandLine::FindOption(TEXT("Test.Plain"), Value));
        TEST_EXPECT(Value.Equals(TEXT("1")));
    }

    TEST_SECTION(TEXT("Windows' splitting keeps non-ASCII text and escaped quotes"));
    {
        TArray<FString> Arguments;
        FWindowsPlatformMisc::SplitCommandLine(L"Editor.exe \"-engine=E:/Motor \u00E5\u00E4\u00F6 \u65E5\u672C\" -ExecuteCommands=\"Echo \\\"Hi\\\"\"", Arguments);

        TEST_EXPECT_EQ(Arguments.Size(), 2);
        if (Arguments.Size() == 2)
        {
            TEST_EXPECT(Arguments[0].Equals(TEXT("-engine=E:/Motor åäö 日本")));
            TEST_EXPECT(Arguments[1].Equals(TEXT("-ExecuteCommands=Echo \"Hi\"")));
        }

        FWindowsPlatformMisc::SplitCommandLine(L"Editor.exe", Arguments);
        TEST_EXPECT(Arguments.IsEmpty());
    }
#endif

    TEST_END();
}
