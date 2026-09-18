include "BuildTool.lua"

-- MetalRHI Module

if IsPlatformMac() then
    local MetalRHI = ModuleBuildRules("MetalRHI")
    MetalRHI.bRuntimeLinking        = true
    MetalRHI.bUsePrecompiledHeaders = true
    
    MetalRHI.AddModules({
        "Core",
        "CoreApplication",
        "RHI",
    })

    MetalRHI.AddFrameworks({
        "Metal",
        "QuartzCore",
    })

    local ToolBinaries         = '/usr/local/bin'
    local DxcExecutable        = ResolveDxcExecutable(ToolBinaries)
    local SpirvCrossExecutable = ResolveSpirvCrossExecutable(ToolBinaries)

    LogHighlight('DXC path=%s', DxcExecutable)
    LogHighlight('spirv-cross path=%s', SpirvCrossExecutable)

    AddGeneratedMSLShaderHeaderRule(MetalRHI, {
        Compiler     = DxcExecutable,
        SpirvCross   = SpirvCrossExecutable,
        Source       = 'Shaders/Internal/ClearBufferUAV.hlsl',
        Arguments    = GetSpirvShaderArguments('cs_6_2'),
        SymbolPrefix = 'GMetalClearBufferUAV_',
        Permutations = {
            { Name = 'Float', Defines = { 'CLEAR_ELEMENT_UINT=0', 'CLEAR_ELEMENT_SINT=0' } },
            { Name = 'Uint',  Defines = { 'CLEAR_ELEMENT_UINT=1', 'CLEAR_ELEMENT_SINT=0' } },
            { Name = 'Sint',  Defines = { 'CLEAR_ELEMENT_UINT=0', 'CLEAR_ELEMENT_SINT=1' } },
        },
    })
end
