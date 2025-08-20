include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- NullRHI Module

local NullRHIModule = ModuleBuildRules("NullRHI")
NullRHIModule.bRuntimeLinking = true

NullRHIModule.AddModuleThirdparties
{
    "Core",
    "RHI",
}
