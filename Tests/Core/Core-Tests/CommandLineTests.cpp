#include "CommandLineTests.h"

#include <Core/Containers/String.h>
#include <Core/Containers/StringView.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

// Windows passes the whole command line as a single unsplit argument 
// (NumArgs == 1), which is the shape most of these cases exercise.
static bool InitializeFromLine(const CHAR* Line)
{
    const CHAR* Args[] = { Line };
    return CommandLine::Initialize(Args, 1);
}

// Real CVar names taken from across the engine, covering every name shape currently in use.
static const CHAR* GNameCorpus[] =
{
    "RHI.Type",
    "RHI.EnableDebugLayer",
    "RHI.EnableValidation",
    "RHI.ShaderCompiler.VerboseLogging",
    "TaskGraph.EnableRHIThread",
    "VulkanRHI.MaxDefragMovesPerFrame",
    "D3D12RHI.ResidencyManager.BudgetPercentage",
    "Renderer.SSAO.Radius",
    "Console.DumpCVars",
    "Echo",
    "Some_Underscored_Name",
    "Mixed.Case_With123.Digits456",
};

bool CommandLine_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Dotted names survive tokenization");
    {
        InitializeFromLine("-RHI.Type=Vulkan");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.Type", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));
    }

    TEST_SECTION("Multi-dotted and underscored names");
    {
        InitializeFromLine("-RHI.ShaderCompiler.VerboseLogging=true -Some_Underscored_Name=7");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.ShaderCompiler.VerboseLogging", Value));
        TEST_EXPECT(Value.Equals("true"));

        TEST_EXPECT(CommandLine::FindOption("Some_Underscored_Name", Value));
        TEST_EXPECT(Value.Equals("7"));
    }

    TEST_SECTION("Value keeps its first character");
    {
        InitializeFromLine("-A.B=Vulkan -C.D=1");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("A.B", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));

        TEST_EXPECT(CommandLine::FindOption("C.D", Value));
        TEST_EXPECT(Value.Equals("1"));
    }

    TEST_SECTION("Floats, negatives and paths are not truncated");
    {
        InitializeFromLine("-A.B=1.5");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("A.B", Value));
        TEST_EXPECT(Value.Equals("1.5"));

        InitializeFromLine("-A.B=-0.5");
        TEST_EXPECT(CommandLine::FindOption("A.B", Value));
        TEST_EXPECT(Value.Equals("-0.5"));

        InitializeFromLine("-Log.File=C:/foo/bar.txt");
        TEST_EXPECT(CommandLine::FindOption("Log.File", Value));
        TEST_EXPECT(Value.Equals("C:/foo/bar.txt"));
    }

    TEST_SECTION("Quoted values may contain spaces");
    {
        InitializeFromLine("-A.B=\"Hello World\" -C.D=2");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("A.B", Value));
        TEST_EXPECT(Value.Equals("Hello World"));

        TEST_EXPECT(CommandLine::FindOption("C.D", Value));
        TEST_EXPECT(Value.Equals("2"));
    }

    TEST_SECTION("Several options on one line");
    {
        InitializeFromLine("-RHI.Type=Vulkan -RHI.EnableDebugLayer=true -Foo.Bar=3");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.Type", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));

        TEST_EXPECT(CommandLine::FindOption("RHI.EnableDebugLayer", Value));
        TEST_EXPECT(Value.Equals("true"));

        TEST_EXPECT(CommandLine::FindOption("Foo.Bar", Value));
        TEST_EXPECT(Value.Equals("3"));
    }

    TEST_SECTION("A dash inside a value does not start a new option");
    {
        InitializeFromLine("-A.B=x-y -C.D=2");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("A.B", Value));
        TEST_EXPECT(Value.Equals("x-y"));

        TEST_EXPECT(CommandLine::FindOption("C.D", Value));
        TEST_EXPECT(Value.Equals("2"));

        TEST_EXPECT(!CommandLine::FindOption("y"));
    }

    TEST_SECTION("Matching is by whole token, not substring");
    {
        InitializeFromLine("-RHI.TypeExtra=1");
        TEST_EXPECT(!CommandLine::FindOption("RHI.Type"));

        // A partial hit must not stop the scan from reaching the real option
        InitializeFromLine("-RHI.TypeExtra=1 -RHI.Type=Vulkan");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.Type", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));

        // A name that only appears as a suffix of another option is not a match
        InitializeFromLine("-Prefixed.RHI.Type=1");
        TEST_EXPECT(!CommandLine::FindOption("RHI.Type"));
    }

    TEST_SECTION("Matching is case-insensitive");
    {
        InitializeFromLine("-RHI.Type=Vulkan");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("rhi.type", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));
    }

    TEST_SECTION("A bare switch is found with an empty value");
    {
        InitializeFromLine("-RHI.EnableDebugLayer -RHI.Type=Vulkan");

        TEST_EXPECT(CommandLine::FindOption("RHI.EnableDebugLayer"));

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.EnableDebugLayer", Value));
        TEST_EXPECT(Value.IsEmpty());

        // The switch must not swallow the option that follows it
        TEST_EXPECT(CommandLine::FindOption("RHI.Type", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));
    }

    TEST_SECTION("A bare switch at the very end of the line");
    {
        InitializeFromLine("-RHI.EnableValidation");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.EnableValidation", Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION("A missing option leaves the output untouched");
    {
        InitializeFromLine("-RHI.Type=Vulkan");

        StringView Value("sentinel");
        TEST_EXPECT(!CommandLine::FindOption("Does.Not.Exist", Value));
        TEST_EXPECT(Value.Equals("sentinel"));
    }

    TEST_SECTION("Argv-style arguments");
    {
        const CHAR* Args[] = { "-RHI.Type=Vulkan", "-RHI.EnableDebugLayer=true" };
        TEST_EXPECT(CommandLine::Initialize(Args, 2));

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("RHI.Type", Value));
        TEST_EXPECT(Value.Equals("Vulkan"));

        TEST_EXPECT(CommandLine::FindOption("RHI.EnableDebugLayer", Value));
        TEST_EXPECT(Value.Equals("true"));

        // Arguments must be separated in the raw copy, not run together
        TEST_EXPECT(StringView(CommandLine::GetOriginal()).Equals("-RHI.Type=Vulkan -RHI.EnableDebugLayer=true"));
    }

    TEST_SECTION("Re-initializing replaces the previous command line");
    {
        InitializeFromLine("-A.Long.Option.Name=SomethingLong -B.Another=2");
        InitializeFromLine("-A.Long.Option.Name=Short");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("A.Long.Option.Name", Value));
        TEST_EXPECT(Value.Equals("Short"));
        TEST_EXPECT(!CommandLine::FindOption("B.Another"));
    }

    TEST_SECTION("An unterminated quote does not assert");
    {
        InitializeFromLine("-A.B=\"Unterminated");

        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("A.B", Value));
        TEST_EXPECT(Value.Equals("Unterminated"));
    }

    TEST_SECTION("A null argument is rejected");
    {
        const CHAR* Args[] = { nullptr };
        TEST_EXPECT(!CommandLine::Initialize(Args, 1));
        TEST_EXPECT(!CommandLine::Initialize(nullptr, 1));
    }

    TEST_SECTION("Every CVar name shape round-trips");
    {
        for (const CHAR* Name : GNameCorpus)
        {
            const String Line = String::CreateFormatted("-%s=ProbeValue", Name);
            InitializeFromLine(*Line);

            StringView Value;
            const bool bRoundTripped = CommandLine::FindOption(Name, Value) && Value.Equals("ProbeValue");
            if (!bRoundTripped)
            {
                LOG_ERROR("CVar name '%s' did not round-trip through the CommandLine", Name);
            }

            TEST_EXPECT(bRoundTripped);
        }
    }

    TEST_SECTION("Generated names over the legal character set");
    {
        const CHAR* Alphabet = "abzABZ019._";
        for (const CHAR* Char = Alphabet; *Char != '\0'; ++Char)
        {
            const String Name = String::CreateFormatted("Prefix%cSuffix", *Char);
            const String Line = String::CreateFormatted("-%s=ProbeValue", *Name);
            InitializeFromLine(*Line);

            StringView Value;
            const bool bRoundTripped = CommandLine::FindOption(*Name, Value) && Value.Equals("ProbeValue");
            if (!bRoundTripped)
            {
                LOG_ERROR("Generated name '%s' did not round-trip through the CommandLine", *Name);
            }

            TEST_EXPECT(bRoundTripped);
        }
    }

    TEST_SECTION("An over-long command line truncates safely");
    {
        String LongLine;
        for (int32 Index = 0; Index < 200; ++Index)
        {
            LongLine.AppendFormat("-Long.Option%d=Value%d ", Index, Index);
        }

        TEST_EXPECT(InitializeFromLine(*LongLine));

        const uint64 Length         = static_cast<uint64>(CString::Strlen(CommandLine::Get()));
        const uint64 OriginalLength = static_cast<uint64>(CString::Strlen(CommandLine::GetOriginal()));
        TEST_EXPECT(Length < CommandLine::MaxCommandLineLength);
        TEST_EXPECT(OriginalLength < CommandLine::MaxCommandLineLength);

        // Whatever did fit has to still be readable
        StringView Value;
        TEST_EXPECT(CommandLine::FindOption("Long.Option0", Value));
        TEST_EXPECT(Value.Equals("Value0"));
    }

    TEST_END();
}
