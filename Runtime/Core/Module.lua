include "BuildTool.lua"

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

    -- TODO: We should set the name of the project and not rely on the target name
    local TargetName = GetCurrentTargetName()
    CoreModule.AddDefines({
        'PROJECT_NAME="' .. TargetName .. '"'
    })

    local UnixProjectPath = path.translate(JoinPath(GetEnginePath(), TargetName), "/")
    local ProjectLocation = 'PROJECT_LOCATION="' .. UnixProjectPath .. '"'
    CoreModule.AddDefines({
        ProjectLocation
    })

    BaseGenerate()
end
