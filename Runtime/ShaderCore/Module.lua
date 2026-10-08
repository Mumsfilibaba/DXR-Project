include "BuildTool.lua"

-- ShaderCore Module

local ShaderCoreModule = ModuleBuildRules("ShaderCore")
ShaderCoreModule.bUsePrecompiledHeaders = true

ShaderCoreModule.AddModules({
    "Core",
})
