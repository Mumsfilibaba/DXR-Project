include "../../BuildScripts/Scripts/build_module.lua"

-- Application Module

local application_module = module_build_rules("Application")
application_module.use_precompiled_headers = true

application_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
    "RHI",
}
