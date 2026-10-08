include "BuildTool.lua"

-- RHI Module

local RhiModule = ModuleBuildRules("RHI")
RhiModule.bUsePrecompiledHeaders = true

RhiModule.AddModules({
    "Core",
    "CoreApplication",
    "ShaderCore",
})
