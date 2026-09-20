include "BuildTool.lua"

local BlueNoiseGen = TargetBuildRules("BlueNoiseGen")
BlueNoiseGen.TargetType = ETargetType.Program
BlueNoiseGen.Kind       = IsPlatformMac() and "WindowedApp" or "ConsoleApp"

BlueNoiseGen.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("tinyddsloader"),
})

BlueNoiseGen.AddModules({
    "Core",
    "CoreApplication",
    "LaunchProgram",
})
