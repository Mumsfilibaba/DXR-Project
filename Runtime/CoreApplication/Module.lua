include "BuildTool.lua"

-- CoreApplication Module

local CoreApplicationModule = ModuleBuildRules("CoreApplication")
CoreApplicationModule.bUsePrecompiledHeaders = true

CoreApplicationModule.AddModules({ "Core" })

if IsPlatformMac() then
    CoreApplicationModule.AddFrameworks({
        "Cocoa",
        "AppKit",
        "IOKit",
        "GameController",
        "UniformTypeIdentifiers",
    })
elseif IsPlatformWindows() then
    CoreApplicationModule.AddLinkLibraries({ "Shcore.lib", "Dwmapi.lib", "Ole32.lib", "Shell32.lib" })
end
