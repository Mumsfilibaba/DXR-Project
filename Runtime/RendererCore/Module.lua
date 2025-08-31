include "BuildTool.lua"

-- RendererCore Module

local RendererCoreModule = ModuleBuildRules("RendererCore")

RendererCoreModule.AddModules({
    "Core",
    "RHI",
})
