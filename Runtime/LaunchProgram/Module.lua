include "BuildTool.lua"

-- LaunchProgram Module

local LaunchProgram = ModuleBuildRules("LaunchProgram")
LaunchProgram.bIsDynamic = false

LaunchProgram.AddModules({
    "Core",
    "CoreApplication",
})

if IsPlatformMac() then
    LaunchProgram.AddFrameworks({
        "Cocoa",
        "AppKit",
    })
end
