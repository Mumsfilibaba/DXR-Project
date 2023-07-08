include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Project Module

local ProjectModule = FModuleBuildRules("Project")
ProjectModule.AddModuleDependencies( 
{
    "Core"
})

-- Base generate (generates project files)
local BuildRulesGenerate = ProjectModule.Generate 
function ProjectModule.Generate()
    if ProjectModule.Workspace == nil then
        LogError("Workspace cannot be nil when generating Rule")
        return
    end

    local TargetName = ProjectModule.Workspace.GetCurrentTargetName()
    ProjectModule.AddDefines({ "PROJECT_NAME=\"" .. TargetName .. "\"" })
    ProjectModule.AddDefines({ "PROJECT_LOCATION=\"" .. ProjectModule.Workspace.GetEnginePath() .. "/" .. TargetName .. "\"" })

    BuildRulesGenerate()
end
