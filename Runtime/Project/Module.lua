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
    ProjectModule.AddDefine("PROJECT_NAME=\"" .. TargetName .. "\"")
    ProjectModule.AddDefine("PROJECT_LOCATION=\"" .. GetEnginePath() .. "/" .. TargetName .. "\"")
else
    ProjectModule.AddDefine("PROJECT_NAME=\"\"")
    ProjectModule.AddDefine("PROJECT_LOCATION=\"" .. GetEnginePath() .. "\"")
end
