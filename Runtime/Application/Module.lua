include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- Application Module

local application_module = module_build_rules("Application")
application_module.use_precompiled_headers = true

application_module.add_module_thirdparties
{
    "Core",
    "CoreApplication",
    "RHI",
}
