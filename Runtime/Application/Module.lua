include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- Application Module

local ApplicationModule = ModuleBuildRules("Application")
ApplicationModule.bUsePrecompiledHeaders = true

ApplicationModule.AddModuleThirdparties
{
    "Core",
    "CoreApplication",
    "RHI",
}