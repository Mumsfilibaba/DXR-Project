include "BuildTool.lua"

-- Core Math Tests
local function ConfigureMathTarget(Target, PinnedDefines, ClangLevel)
    Target.TargetType = ETargetType.Program
    Target.Kind       = "ConsoleApp"

    Target.AddModules({
        "Core",
        "TestCommon",
    })

    Target.AddDefines(PinnedDefines)

    -- MSVC hardcodes every __SSE*__ macro on x64, so the defines above do the pinning there and
    -- /arch has to stay at the default. Clang only defines the macros the -m flag enables, so
    -- macOS needs the matching level or the lower variants all collapse onto clang's baseline.
    Target.VectorExtensions = IsPlatformMac() and ClangLevel or "Default"
end

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-Scalar"),
{
    "PLATFORM_SUPPORT_SSE_INTRIN=0",
}, "Default")

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-SSE"),
{
    "PLATFORM_SUPPORT_SSE2_INTRIN=0",
    "PLATFORM_SUPPORT_SSE3_INTRIN=0",
    "PLATFORM_SUPPORT_SSSE3_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_1_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_2_INTRIN=0",
}, "SSE")

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-SSE2"),
{
    "PLATFORM_SUPPORT_SSE3_INTRIN=0",
    "PLATFORM_SUPPORT_SSSE3_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_1_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_2_INTRIN=0",
}, "SSE2")

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-SSE3"),
{
    "PLATFORM_SUPPORT_SSSE3_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_1_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_2_INTRIN=0",
}, "SSE3")

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-SSSE3"),
{
    "PLATFORM_SUPPORT_SSE4_1_INTRIN=0",
    "PLATFORM_SUPPORT_SSE4_2_INTRIN=0",
}, "SSSE3")

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-SSE4_1"),
{
    "PLATFORM_SUPPORT_SSE4_2_INTRIN=0",
}, "SSE4.1")

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-SSE4_2"), {}, "SSE4.2")
