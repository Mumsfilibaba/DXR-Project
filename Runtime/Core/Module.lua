include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Core Module

local CoreModule = CModuleBuildRules('Core')
CoreModule.bUsePrecompiledHeaders = true
CoreModule.Generate()