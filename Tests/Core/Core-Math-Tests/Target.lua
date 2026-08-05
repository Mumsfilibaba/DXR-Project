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
    Target.VectorExtensions = IsPlatformMac() and ClangLevel or "Default"
end

ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-Scalar"),
{
    TargetsX86() and "PLATFORM_SUPPORT_SSE_INTRIN=0" or "PLATFORM_SUPPORT_NEON_INTRIN=0",
}, "Default")

if TargetsX86() then
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
else
    ConfigureMathTarget(TargetBuildRules("Core-Math-Tests-NEON"), {}, "Default")
end
