include "BuildTool.lua"

-- Application Module

local ApplicationModule = ModuleBuildRules("Application")
ApplicationModule.bUsePrecompiledHeaders = true

ApplicationModule.AddModules({
    "Core",
    "CoreApplication",
    "RHI",
})