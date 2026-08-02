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
        "User32.lib",
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

    -- The target knows where its own Target.lua lives; targets outside <Engine>/<Name>
    -- (the test suites, for one) would otherwise get a path that does not exist.
    local TargetRule      = GetTargetRule(TargetName)
    local TargetPath      = TargetRule and TargetRule.GetPath() or JoinPath(GetEnginePath(), TargetName)
    local UnixProjectPath = path.translate(TargetPath, "/")
    local ProjectLocation = 'PROJECT_LOCATION="' .. UnixProjectPath .. '"'
    CoreModule.AddDefines({
        ProjectLocation
    })

    BaseGenerate()
end
