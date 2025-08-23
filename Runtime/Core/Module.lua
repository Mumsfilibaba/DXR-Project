include "BuildTool_Module.lua"

-- Core Module

local CoreModule = ModuleBuildRules("Core")
CoreModule.bUsePrecompiledHeaders = true

if IsPlatformMac() then
    CoreModule.AddFrameworks({
        "AppKit",
    })
elseif IsPlatformWindows() then
    CoreModule.AddLinkLibraries({
        "Dbghelp.lib",
        "shlwapi.lib",
    })
end

-- Add project name to the core module
local BaseGenerate = CoreModule.Generate
function CoreModule.Generate()
    if CoreModule.Workspace == nil then
        LogError("Workspace cannot be nil when generating Rule")
        return
    end

    local TargetName = CoreModule.Workspace.GetCurrentTargetName()
    CoreModule.AddDefines({ 'PROJECT_NAME="' .. TargetName .. '"' })

    local UnixProjectPath = path.translate(JoinPath(GetEnginePath(), TargetName), "/")
    local ProjectLocation = 'PROJECT_LOCATION="' .. UnixProjectPath .. '"'
    CoreModule.AddDefines({ ProjectLocation })

    BaseGenerate()
end
