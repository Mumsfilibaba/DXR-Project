include '../../BuildScripts/Scripts/build_module.lua'

local CoreModule = CModuleBuildRules('Core')
CoreModule.bUsePrecompiledHeaders = true
CoreModule.Generate()