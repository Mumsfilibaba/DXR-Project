include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = CModuleBuildRules('RHI')

RHIModule.AddSystemIncludes(
{
    CreateExternalDependencyPath('DXC/include')
})

RHIModule.AddLibraryPaths(
{
    CreateExternalDependencyPath('DXC/bin')
})

RHIModule.AddModuleDependencies( 
{
    'Core',
})

RHIModule.Generate()