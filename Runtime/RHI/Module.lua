include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = FModuleBuildRules("RHI")

RHIModule.AddSystemIncludes(
{
    CreateExternalDependencyPath("DXC/include"),
    CreateExternalDependencyPath("SPIRV-Cross"),
    CreateExternalDependencyPath("glslang/include")
})

RHIModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication"
})

if IsPlatformMac() then
    RHIModule.AddLibraryPaths( 
    {
        CreateExternalDependencyPath("glslang/lib/macOS"),
    })

    RHIModule.AddLinkLibraries(
    {
        "GenericCodeGen",
        "SPIRV-Tools",
        "HLSL",
        "SPIRV",
        "MachineIndependent",
        "SPVRemapper",
        "OGLCompiler",
        "glslang-default-resource-limits",
        "OSDependent",
        "glslang",
        "SPIRV-Tools-opt",
    })
elseif IsPlatformWindows() then
    RHIModule.AddLibraryPaths( 
    {
        CreateExternalDependencyPath("glslang/lib/Windows"),
    })

    RHIModule.AddLinkLibraries(
    {
        "GenericCodeGen.lib",
        "SPIRV-Tools.lib",
        "HLSL.lib",
        "SPIRV.lib",
        "MachineIndependent.lib",
        "SPVRemapper.lib",
        "OGLCompiler.lib",
        "glslang-default-resource-limits.lib",
        "OSDependent.lib",
        "glslang.lib",
        "SPIRV-Tools-opt.lib",
    })
end

RHIModule.AddLinkLibraries( 
{
    "SPIRV-Cross",
})
