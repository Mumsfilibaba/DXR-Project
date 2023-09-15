include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = FModuleBuildRules("RHI")

RHIModule.AddSystemIncludes(
{
    CreateExternalDependencyPath("DXC/include"),
    CreateExternalDependencyPath("SPIRV-Cross"),
    CreateExternalDependencyPath("glslang")
})

RHIModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication"
})

RHIModule.AddLinkLibraries(
{
    --"GenericCodeGen",
    --"SPIRV-Tools",
    --"HLSL",
    "SPIRV",
    "MachineIndependent",
    "SPVRemapper",
    "OGLCompiler",
    "glslang-default-resource-limits",
    --"OSDependent",
    "glslang",
    --"SPIRV-Tools-opt",
})

RHIModule.AddLinkLibraries( 
{
    "SPIRV-Cross",
})
