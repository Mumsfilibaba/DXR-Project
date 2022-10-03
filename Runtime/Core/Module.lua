include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Core Module

local CoreModule = FModuleBuildRules('Core')
CoreModule.bUsePrecompiledHeaders = true

if BuildWithXcode() then
    CoreModule.AddFrameWorks( 
    {
        'AppKit',
    })
end

CoreModule.Generate()