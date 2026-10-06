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
        "CoreGraphics",
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
            { Name = 'Raw',   Defines = { 'CLEAR_ELEMENT_RAW=1' } },
        },
    })

    -- One permutation per UAV texture type and component type, named <Dimension>_<Component>
    local TextureClearPermutations = {}
    local ClearDimensions =
    {
        { Name = 'Texture1D',      Value = 0 },
        { Name = 'Texture1DArray', Value = 1 },
        { Name = 'Texture2D',      Value = 2 },
        { Name = 'Texture2DArray', Value = 3 },
        { Name = 'Texture3D',      Value = 4 },
    }
    local ClearComponents =
    {
        { Name = 'Float', Uint = 0, Sint = 0 },
        { Name = 'Uint',  Uint = 1, Sint = 0 },
        { Name = 'Sint',  Uint = 0, Sint = 1 },
    }

    for _, Dimension in ipairs(ClearDimensions) do
        for _, Component in ipairs(ClearComponents) do
            table.insert(TextureClearPermutations, {
                Name    = Dimension.Name .. '_' .. Component.Name,
                Defines =
                {
                    'CLEAR_DIMENSION=' .. Dimension.Value,
                    'CLEAR_ELEMENT_UINT=' .. Component.Uint,
                    'CLEAR_ELEMENT_SINT=' .. Component.Sint,
                },
            })
        end
    end

    AddGeneratedMSLShaderHeaderRule(MetalRHI, {
        Compiler     = DxcExecutable,
        SpirvCross   = SpirvCrossExecutable,
        Source       = 'Shaders/Internal/ClearTextureUAV.hlsl',
        Arguments    = GetSpirvShaderArguments('cs_6_2'),
        SymbolPrefix = 'GMetalClearTextureUAV_',
        Permutations = TextureClearPermutations,
    })
end
