include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = CModuleBuildRules('RHI')
RHIModule.AddModuleDependencies( 
{
    'Core',
})

RHIModule.Generate()