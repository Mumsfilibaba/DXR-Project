include "BuildTool.lua"

local RemoteConsole = TargetBuildRules("RemoteConsole")
RemoteConsole.TargetType = ETargetType.Program
RemoteConsole.Kind       = IsPlatformMac() and "WindowedApp" or "ConsoleApp"

RemoteConsole.AddModules({
    "Core",
    "CoreApplication",
    "LaunchProgram",
})
