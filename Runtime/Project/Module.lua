include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Project Module

local ProjectModule = FModuleBuildRules("Project")
ProjectModule.AddModuleDependencies( 
{
    "Core"
})

-- TODO: A better way would probably be to use a config file for this
if GetCurrentTargetName ~= nil then
    local TargetName = GetCurrentTargetName()
    ProjectModule.AddDefines(
    {
        ("PROJECT_NAME=\"" .. TargetName .. "\""),
        ("PROJECT_LOCATION=\"" .. FindWorkspaceDir() .. "/" .. TargetName .. "\""),
    })
else
    ProjectModule.AddDefines(
    {
        ("PROJECT_NAME=\"\""),
        ("PROJECT_LOCATION=\"" .. FindWorkspaceDir() .. "\""),
    })
end

ProjectModule.Generate()