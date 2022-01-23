include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- CoreModule

local CoreModule = CModuleBuildRules('Core')
CoreModule.bUsePrecompiledHeaders = true
CoreModule.Generate()