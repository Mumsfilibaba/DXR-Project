include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Core Module

local CoreModule = FModuleBuildRules('Core')
CoreModule.bUsePrecompiledHeaders = true
CoreModule.Generate()