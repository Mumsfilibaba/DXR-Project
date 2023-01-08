
include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- Application Module

local ApplicationModule = FModuleBuildRules("Application")
ApplicationModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath("imgui")
})

ApplicationModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "RHI",
})

ApplicationModule.AddLinkLibraries( 
{
    "ImGui",
})

if BuildWithXcode() then
    ApplicationModule.AddFrameWorks( 
    {
        "AppKit",
    })
end

ApplicationModule.Generate()