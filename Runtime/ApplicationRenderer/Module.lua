include "BuildTool.lua"

-- ApplicationRenderer Module

local ApplicationRendererModule = ModuleBuildRules("ApplicationRenderer")

ApplicationRendererModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
})
