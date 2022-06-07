include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = CModuleBuildRules('RHI')

RHIModule.AddSystemIncludes(
{
    CreateExternalDependencyPath('DXC/include'),
    CreateExternalDependencyPath('SPIRV-Cross')
})

RHIModule.AddModuleDependencies( 
{
    'Core',
})

RHIModule.AddLinkLibraries( 
{
    'SPIRV-Cross',
})

RHIModule.Generate()