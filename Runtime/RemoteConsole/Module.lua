include "BuildTool.lua"

-- RemoteConsole Module

local RemoteConsoleModule = ModuleBuildRules("RemoteConsole")
RemoteConsoleModule.bUsePrecompiledHeaders = true

RemoteConsoleModule.AddModules({
    "Core",
})
