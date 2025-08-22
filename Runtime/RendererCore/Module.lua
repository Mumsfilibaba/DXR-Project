include "BuildTool_Module.lua"

-- RendererCore Module

local RendererCoreModule = ModuleBuildRules("RendererCore")

RendererCoreModule.AddModules({
    "Core",
    "RHI",
})
