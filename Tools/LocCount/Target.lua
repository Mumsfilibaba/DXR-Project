include "BuildTool.lua"

local LocCount = TargetBuildRules("LocCount")
LocCount.TargetType = ETargetType.Program
LocCount.Kind       = IsPlatformMac() and "WindowedApp" or "ConsoleApp"

LocCount.AddModules({
    "Core",
    "CoreApplication",
    "LaunchProgram",
})
