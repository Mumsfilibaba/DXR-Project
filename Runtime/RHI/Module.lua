include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = FModuleBuildRules('RHI')

RHIModule.AddSystemIncludes(
{
    CreateExternalDependencyPath('DXC/include'),
    CreateExternalDependencyPath('SPIRV-Cross')
})

RHIModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication'
})

RHIModule.AddLinkLibraries( 
{
    'SPIRV-Cross',
})

RHIModule.Generate()